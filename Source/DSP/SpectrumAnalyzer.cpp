/*
  ==============================================================================

    SpectrumAnalyzer.cpp
    Created: 12 Sep 2026 6:33:01am
    Author:  Maximos Kaliakatsos-Papakostas

  ==============================================================================
*/

/*
  ==============================================================================

    SpectrumAnalyzer.cpp

  ==============================================================================
*/

#include "SpectrumAnalyzer.h"

#include <algorithm>
#include <cmath>


//==============================================================================
// Construction
//==============================================================================

SpectrumAnalyzer::SpectrumAnalyzer(
    double initialSampleRate)
    : fft(fftOrder),
      window(
          fftSize,
          juce::dsp::WindowingFunction<float>::hann,
          true),
      sampleRate(initialSampleRate)
{
    calculateBands();
    updateSmoothingCoefficients();
    reset();
}


//==============================================================================
// Setup
//==============================================================================

void SpectrumAnalyzer::setSampleRate(
    double newSampleRate)
{
    if (newSampleRate <= 0.0)
        return;

    sampleRate = newSampleRate;

    calculateBands();
    updateSmoothingCoefficients();
    reset();
}


void SpectrumAnalyzer::reset()
{
    sampleBuffer.fill(0.0f);
    fftData.fill(0.0f);

    targetSpectrum.fill(minDB);
    smoothedSpectrum.fill(minDB);

    sampleBufferPosition = 0;
    samplesUntilFFT = hopSize;
}


//==============================================================================
// Audio processing
//==============================================================================

void SpectrumAnalyzer::processBlock(
    const float* samples,
    int numSamples) noexcept
{
    if (samples == nullptr || numSamples <= 0)
        return;


    for (int i = 0; i < numSamples; ++i)
    {
        sampleBuffer[
            static_cast<size_t>(sampleBufferPosition)] =
            samples[i];

        sampleBufferPosition =
            (sampleBufferPosition + 1) % fftSize;

        --samplesUntilFFT;

        if (samplesUntilFFT <= 0)
        {
            performFFT();

            samplesUntilFFT = hopSize;
        }
    }
}


//==============================================================================
// FFT
//==============================================================================

void SpectrumAnalyzer::performFFT() noexcept
{
    // ================================================================
    // Copy the circular buffer into chronological order.
    // ================================================================

    for (int i = 0; i < fftSize; ++i)
    {
        const int index =
            (sampleBufferPosition + i) % fftSize;

        fftData[static_cast<size_t>(i)] =
            sampleBuffer[static_cast<size_t>(index)];
    }


    // The second half must be zeroed because the FFT function
    // operates on a 2N-sized buffer.
    std::fill(
        fftData.begin() + fftSize,
        fftData.end(),
        0.0f);


    // ================================================================
    // Window
    // ================================================================

    window.multiplyWithWindowingTable(
        fftData.data(),
        fftSize);


    // ================================================================
    // FFT
    // ================================================================

    fft.performFrequencyOnlyForwardTransform(
        fftData.data());


    // ================================================================
    // Convert FFT result into our 64 display bands.
    // ================================================================

    calculateSpectrum();
}


//==============================================================================
// Spectrum calculation
//==============================================================================

void SpectrumAnalyzer::calculateSpectrum() noexcept
{
    for (int bandIndex = 0;
         bandIndex < numSpectrumBands;
         ++bandIndex)
    {
        const Band& band =
            bands[static_cast<size_t>(bandIndex)];


        // ------------------------------------------------------------
        // Protect against pathological cases where a band contains
        // no FFT bins.
        // ------------------------------------------------------------

        if (band.lastFFTBin < band.firstFFTBin)
        {
            targetSpectrum[
                static_cast<size_t>(bandIndex)] =
                minDB;

            continue;
        }


        double power = 0.0;

        int numberOfBins = 0;


        // ------------------------------------------------------------
        // Accumulate power.
        // ------------------------------------------------------------

        for (int bin = band.firstFFTBin;
             bin <= band.lastFFTBin;
             ++bin)
        {
            if (bin < 0 || bin >= fftSize / 2)
                continue;


            const float magnitude =
                fftData[static_cast<size_t>(bin)];


            // JUCE's frequency-only FFT returns magnitude.
            //
            // Normalize by FFT size.
            //
            // Positive-frequency bins are multiplied by 2 to account
            // for the discarded negative-frequency half.
            //
            // The exact absolute calibration is not intended to be
            // a precision SPL meter; it gives us a useful dBFS-like
            // spectrum for visualization.

            float normalizedMagnitude =
                magnitude /
                static_cast<float>(fftSize);

            if (bin != 0)
                normalizedMagnitude *= 2.0f;


            power +=
                static_cast<double>(
                    normalizedMagnitude) *
                static_cast<double>(
                    normalizedMagnitude);

            ++numberOfBins;
        }


        if (numberOfBins > 0)
        {
            // Average the power represented by the band.
            power /= static_cast<double>(numberOfBins);
        }


        const float levelDB =
            power > 1.0e-20
                ? 10.0f *
                  std::log10(
                      static_cast<float>(power))
                : minDB;


        targetSpectrum[
            static_cast<size_t>(bandIndex)] =
            juce::jlimit(
                minDB,
                maxDB,
                levelDB);
    }


    // ================================================================
    // Attack/release smoothing
    // ================================================================

    for (int i = 0;
         i < numSpectrumBands;
         ++i)
    {
        const size_t index =
            static_cast<size_t>(i);

        const float target =
            targetSpectrum[index];

        float& current =
            smoothedSpectrum[index];


        if (target > current)
        {
            // Fast attack.
            current =
                target +
                attackCoefficient *
                (current - target);
        }
        else
        {
            // Slow release.
            current =
                target +
                releaseCoefficient *
                (current - target);
        }
    }
}


//==============================================================================
// Frequency bands
//==============================================================================

void SpectrumAnalyzer::calculateBands()
{
    const double minLog =
        std::log10(
            static_cast<double>(minFrequency));

    const double maxLog =
        std::log10(
            static_cast<double>(maxFrequency));


    for (int i = 0;
         i < numSpectrumBands;
         ++i)
    {
        const double t0 =
            static_cast<double>(i) /
            static_cast<double>(numSpectrumBands);

        const double t1 =
            static_cast<double>(i + 1) /
            static_cast<double>(numSpectrumBands);

        const double tc =
            (t0 + t1) * 0.5;


        const float low =
            static_cast<float>(
                std::pow(
                    10.0,
                    minLog +
                    t0 * (maxLog - minLog)));


        const float high =
            static_cast<float>(
                std::pow(
                    10.0,
                    minLog +
                    t1 * (maxLog - minLog)));


        const float centre =
            static_cast<float>(
                std::pow(
                    10.0,
                    minLog +
                    tc * (maxLog - minLog)));


        const int firstBin =
            static_cast<int>(
                std::ceil(
                    frequencyToFFTBin(
                        low,
                        sampleRate)));


        const int lastBin =
            static_cast<int>(
                std::floor(
                    frequencyToFFTBin(
                        high,
                        sampleRate)));


        bands[static_cast<size_t>(i)] =
        {
            low,
            high,
            centre,
            firstBin,
            lastBin
        };
    }
}


//==============================================================================
// Smoothing
//==============================================================================

void SpectrumAnalyzer::updateSmoothingCoefficients()
{
    // The spectrum is updated every hopSize samples.
    //
    // Convert the desired attack/release time into a coefficient
    // suitable for exponential smoothing.

    const float updateInterval =
        static_cast<float>(
            hopSize / sampleRate);


    attackCoefficient =
        std::exp(
            -updateInterval /
            attackTimeSeconds);


    releaseCoefficient =
        std::exp(
            -updateInterval /
            releaseTimeSeconds);
}


//==============================================================================
// Results
//==============================================================================

SpectrumAnalyzer::Spectrum
SpectrumAnalyzer::getSpectrum() const noexcept
{
    return smoothedSpectrum;
}


//==============================================================================
// Frequency information
//==============================================================================

float SpectrumAnalyzer::getBandCentreFrequency(
    int band) noexcept
{
    if (band < 0 || band >= numSpectrumBands)
        return minFrequency;

    const double minLog =
        std::log10(
            static_cast<double>(minFrequency));

    const double maxLog =
        std::log10(
            static_cast<double>(maxFrequency));

    const double t =
        (static_cast<double>(band) + 0.5) /
        static_cast<double>(numSpectrumBands);

    return static_cast<float>(
        std::pow(
            10.0,
            minLog + t * (maxLog - minLog)));
}


float SpectrumAnalyzer::getBandLowFrequency(
    int band) noexcept
{
    if (band < 0 || band >= numSpectrumBands)
        return minFrequency;

    const double minLog =
        std::log10(
            static_cast<double>(minFrequency));

    const double maxLog =
        std::log10(
            static_cast<double>(maxFrequency));

    const double t =
        static_cast<double>(band) /
        static_cast<double>(numSpectrumBands);

    return static_cast<float>(
        std::pow(
            10.0,
            minLog + t * (maxLog - minLog)));
}


float SpectrumAnalyzer::getBandHighFrequency(
    int band) noexcept
{
    if (band < 0 || band >= numSpectrumBands)
        return maxFrequency;

    const double minLog =
        std::log10(
            static_cast<double>(minFrequency));

    const double maxLog =
        std::log10(
            static_cast<double>(maxFrequency));

    const double t =
        static_cast<double>(band + 1) /
        static_cast<double>(numSpectrumBands);

    return static_cast<float>(
        std::pow(
            10.0,
            minLog + t * (maxLog - minLog)));
}


//==============================================================================
// Helpers
//==============================================================================

float SpectrumAnalyzer::frequencyToFFTBin(
    float frequency,
    double sampleRate) noexcept
{
    return frequency *
           static_cast<float>(fftSize) /
           static_cast<float>(sampleRate);
}
