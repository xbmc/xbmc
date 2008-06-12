// ---------------------------------------------------------------------------------------

#include <iostream>
#include <cmath>
#include <cassert>
#include <cstddef>
#include "portaudiocpp/PortAudioCpp.hxx"

// ---------------------------------------------------------------------------------------

// Some constants:
const int NUM_SECONDS = 5;
const double SAMPLE_RATE = 44100.0;
const int FRAMES_PER_BUFFER = 64;
const int TABLE_SIZE = 200;

// ---------------------------------------------------------------------------------------

// SineGenerator class:
class SineGenerator
{
public:
	SineGenerator(int tableSize) : tableSize_(tableSize), leftPhase_(0), rightPhase_(0)
	{
		const double PI = 3.14159265;
		table_ = new float[tableSize];
		for (int i = 0; i < tableSize; ++i)
		{
			table_[i] = 0.125f * (float)sin(((double)i/(double)tableSize)*PI*2.);
		}
	}

	~SineGenerator()
	{
		delete[] table_;
	}

	int generate(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, 
		const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags)
	{
		assert(outputBuffer != NULL);

		float **out = static_cast<float **>(outputBuffer);

		for (unsigned int i = 0; i < framesPerBuffer; ++i)
		{
			out[0][i] = table_[leftPhase_];
			out[1][i] = table_[rightPhase_];

			leftPhase_ += 1;
			if (leftPhase_ >= tableSize_)
				leftPhase_ -= tableSize_;

			rightPhase_ += 3;
			if (rightPhase_ >= tableSize_)
				rightPhase_ -= tableSize_;
		}

		return paContinue;
	}

private:
	float *table_;
	int tableSize_;
	int leftPhase_;
	int rightPhase_;
};

// ---------------------------------------------------------------------------------------

// main:
int main(int, char *[]);
int main(int, char *[])
{
	try
	{
		// Create a SineGenerator object:
		SineGenerator sineGenerator(TABLE_SIZE);

		std::cout << "Setting up PortAudio..." << std::endl;

		// Set up the System:
		portaudio::AutoSystem autoSys;
		portaudio::System &sys = portaudio::System::instance();

		// Set up the parameters required to open a (Callback)Stream:
		portaudio::DirectionSpecificStreamParameters outParams(sys.defaultOutputDevice(), 2, portaudio::FLOAT32, false, sys.defaultOutputDevice().defaultLowOutputLatency(), NULL);
		portaudio::StreamParameters params(portaudio::DirectionSpecificStreamParameters::null(), outParams, SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff);

		std::cout << "Opening stereo output stream..." << std::endl;

		// Create (and open) a new Stream, using the SineGenerator::generate function as a callback:
		portaudio::MemFunCallbackStream<SineGenerator> stream(params, sineGenerator, &SineGenerator::generate);

		std::cout << "Starting playback for " << NUM_SECONDS << " seconds." << std::endl;

		// Start the Stream (audio playback starts):
		stream.start();

		// Wait for 5 seconds:
		sys.sleep(NUM_SECONDS * 1000);

		std::cout << "Closing stream..." <<std::endl;

		// Stop the Stream (not strictly needed as termintating the System will also stop all open Streams):
		stream.stop();

		// Close the Stream (not strictly needed as terminating the System will also close all open Streams):
		stream.close();

		// Terminate the System (not strictly needed as the AutoSystem will also take care of this when it 
		// goes out of scope):
		sys.terminate();

		std::cout << "Test finished." << std::endl;
	}
	catch (const portaudio::PaException &e)
	{
		std::cout << "A PortAudio error occured: " << e.paErrorText() << std::endl;
	}
	catch (const portaudio::PaCppException &e)
	{
		std::cout << "A PortAudioCpp error occured: " << e.what() << std::endl;
	}
	catch (const std::exception &e)
	{
		std::cout << "A generic exception occured: " << e.what() << std::endl;
	}
	catch (...)
	{
		std::cout << "An unknown exception occured." << std::endl;
	}

	return 0;
}


