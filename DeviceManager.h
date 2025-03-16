#pragma once

#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include <string>

// Forward declarations
struct AudioDeviceInfo;
struct MidiDeviceInfo;

class DeviceManager {
public:
    DeviceManager();
    ~DeviceManager();

    // Audio device management
    std::vector<AudioDeviceInfo> EnumerateAudioInputDevices() const;
    std::vector<AudioDeviceInfo> EnumerateAudioOutputDevices() const;
    bool ConnectAudioInputToOutput(const AudioDeviceInfo& input, const AudioDeviceInfo& output);
    void DisconnectAudioDevices();

    // MIDI device management
    std::vector<MidiDeviceInfo> EnumerateMidiInputDevices() const;
    std::vector<MidiDeviceInfo> EnumerateMidiOutputDevices() const;
    bool ConnectMidiInputToOutput(const MidiDeviceInfo& input, const MidiDeviceInfo& output);
    void DisconnectMidiDevices();

private:
    // Audio device connection state
    HWAVEIN m_hWaveIn;
    HWAVEOUT m_hWaveOut;
    bool m_audioConnected;
    volatile bool m_isShuttingDown;  // Flag to indicate shutdown in progress

    // Audio buffer management
    static const int NUM_BUFFERS = 4;
    static const int BUFFER_SIZE = 4096; // 2KB per buffer
    struct AudioBuffer {
        WAVEHDR inHeader;
        WAVEHDR outHeader;
        BYTE inData[BUFFER_SIZE];
        BYTE outData[BUFFER_SIZE];
        volatile bool inUse;  // Track if buffer is currently being processed
    };
    std::vector<AudioBuffer> m_audioBuffers;
    int m_currentBuffer;

    // Audio callback handling
    static void CALLBACK WaveInProc(HWAVEIN hWaveIn, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    void HandleAudioData(LPWAVEHDR lpWaveHdr);

    // MIDI device connection state
    HMIDIIN m_hMidiIn;
    HMIDIOUT m_hMidiOut;
    bool m_midiConnected;

    // MIDI callback handling
    static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    void HandleMidiMessage(DWORD_PTR dwParam1, DWORD_PTR dwParam2);

    // Helper functions
    static std::wstring GetDeviceName(UINT deviceId, bool isInput);
    static bool IsDeviceAvailable(UINT deviceId, bool isInput);
};

struct AudioDeviceInfo {
    UINT deviceId;
    std::wstring name;
    bool isInput;
};

struct MidiDeviceInfo {
    UINT deviceId;
    std::wstring name;
    bool isInput;
}; 