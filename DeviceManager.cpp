#include "DeviceManager.h"
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <endpointvolume.h>

static void LogMessage(LPCWSTR message)
{
    // You can put a breakpoint/watch here or uncomment the following line to see the messages
    // OutputDebugStringW(message);
}

DeviceManager::DeviceManager()
    : m_hWaveIn(nullptr)
    , m_hWaveOut(nullptr)
    , m_audioConnected(false)
    , m_isShuttingDown(false)
    , m_hMidiIn(nullptr)
    , m_hMidiOut(nullptr)
    , m_midiConnected(false)
{
}

DeviceManager::~DeviceManager()
{
    DisconnectAudioDevices();
    DisconnectMidiDevices();
}

std::vector<AudioDeviceInfo> DeviceManager::EnumerateAudioInputDevices() const
{
    std::vector<AudioDeviceInfo> devices;
    
    // Add "No Device" option
    AudioDeviceInfo noDevice;
    noDevice.deviceId = WAVE_MAPPER;
    noDevice.name = L"No Device";
    noDevice.isInput = true;
    devices.push_back(noDevice);

    // Enumerate wave input devices
    UINT numDevices = waveInGetNumDevs();
    for (UINT i = 0; i < numDevices; i++)
    {
        WAVEINCAPSW caps;
        if (waveInGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
        {
            AudioDeviceInfo device;
            device.deviceId = i;
            device.name = caps.szPname;
            device.isInput = true;
            devices.push_back(device);
        }
    }

    return devices;
}

std::vector<AudioDeviceInfo> DeviceManager::EnumerateAudioOutputDevices() const
{
    std::vector<AudioDeviceInfo> devices;
    
    // Add "No Device" option
    AudioDeviceInfo noDevice;
    noDevice.deviceId = WAVE_MAPPER;
    noDevice.name = L"No Device";
    noDevice.isInput = false;
    devices.push_back(noDevice);

    // Enumerate wave output devices
    UINT numDevices = waveOutGetNumDevs();
    for (UINT i = 0; i < numDevices; i++)
    {
        WAVEOUTCAPSW caps;
        if (waveOutGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
        {
            AudioDeviceInfo device;
            device.deviceId = i;
            device.name = caps.szPname;
            device.isInput = false;
            devices.push_back(device);
        }
    }

    return devices;
}

bool DeviceManager::ConnectAudioInputToOutput(const AudioDeviceInfo& input, const AudioDeviceInfo& output)
{
    LogMessage(L"\nConnecting audio devices...");
    LogMessage((L"\nInput: " + input.name).c_str());
    LogMessage((L"\nOutput: " + output.name).c_str());

    m_isShuttingDown = false;

    if (m_audioConnected)
    {
        LogMessage(L"\nDisconnecting existing devices first");
        DisconnectAudioDevices();
    }

    if (input.deviceId == WAVE_MAPPER || output.deviceId == WAVE_MAPPER)
    {
        LogMessage(L"\nInvalid device selection (WAVE_MAPPER)");
        return false;
    }

    // Configure wave format
    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 2;
    wfx.nSamplesPerSec = 44100;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    LogMessage(L"\nOpening input device...");
    // Open wave input device with callback
    MMRESULT result = waveInOpen(&m_hWaveIn, input.deviceId, &wfx, (DWORD_PTR)WaveInProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR)
    {
        LogMessage(L"\nFailed to open input device");
        return false;
    }

    LogMessage(L"\nOpening output device...");
    // Open wave output device
    result = waveOutOpen(&m_hWaveOut, output.deviceId, &wfx, 0, 0, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR)
    {
        LogMessage(L"\nFailed to open output device");
        waveInClose(m_hWaveIn);
        m_hWaveIn = nullptr;
        return false;
    }

    LogMessage(L"\nInitializing audio buffers...");
    // Initialize audio buffers
    m_audioBuffers.resize(NUM_BUFFERS);
    for (int i = 0; i < NUM_BUFFERS; i++)
    {
        wchar_t debugMsg[256];
        swprintf_s(debugMsg, L"\nInitializing buffer %d", i);
        LogMessage(debugMsg);

        // Initialize the buffer structures
        ZeroMemory(&m_audioBuffers[i].inHeader, sizeof(WAVEHDR));
        ZeroMemory(&m_audioBuffers[i].outHeader, sizeof(WAVEHDR));
        ZeroMemory(m_audioBuffers[i].inData, BUFFER_SIZE);
        ZeroMemory(m_audioBuffers[i].outData, BUFFER_SIZE);
        m_audioBuffers[i].inUse = false;
        
        // Set up the input header
        m_audioBuffers[i].inHeader.lpData = (LPSTR)m_audioBuffers[i].inData;
        m_audioBuffers[i].inHeader.dwBufferLength = BUFFER_SIZE;
        m_audioBuffers[i].inHeader.dwUser = i;  // Store buffer index for tracking
        m_audioBuffers[i].inHeader.dwFlags = 0;
        m_audioBuffers[i].inHeader.dwLoops = 0;

        // Set up the output header
        m_audioBuffers[i].outHeader.lpData = (LPSTR)m_audioBuffers[i].outData;
        m_audioBuffers[i].outHeader.dwBufferLength = BUFFER_SIZE;
        m_audioBuffers[i].outHeader.dwUser = i;  // Store buffer index for tracking
        m_audioBuffers[i].outHeader.dwFlags = 0;
        m_audioBuffers[i].outHeader.dwLoops = 0;

        // Prepare headers
        result = waveInPrepareHeader(m_hWaveIn, &m_audioBuffers[i].inHeader, sizeof(WAVEHDR));
        if (result != MMSYSERR_NOERROR)
        {
            LogMessage(L"\nFailed to prepare input header");
            DisconnectAudioDevices();
            return false;
        }

        result = waveOutPrepareHeader(m_hWaveOut, &m_audioBuffers[i].outHeader, sizeof(WAVEHDR));
        if (result != MMSYSERR_NOERROR)
        {
            LogMessage(L"\nFailed to prepare output header");
            DisconnectAudioDevices();
            return false;
        }

        // Add buffer to input queue
        result = waveInAddBuffer(m_hWaveIn, &m_audioBuffers[i].inHeader, sizeof(WAVEHDR));
        if (result != MMSYSERR_NOERROR)
        {
            swprintf_s(debugMsg, L"\nFailed to add buffer to input queue, error: %d", result);
            LogMessage(debugMsg);
            DisconnectAudioDevices();
            return false;
        }
    }

    LogMessage(L"\nStarting recording...");
    // Start recording
    result = waveInStart(m_hWaveIn);
    if (result != MMSYSERR_NOERROR)
    {
        LogMessage(L"\nFailed to start recording");
        DisconnectAudioDevices();
        return false;
    }

    m_currentBuffer = 0;
    m_audioConnected = true;
    LogMessage(L"\nAudio devices connected successfully");
    return true;
}

void CALLBACK DeviceManager::WaveInProc(HWAVEIN hWaveIn, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    if (uMsg == WIM_DATA)
    {
        DeviceManager* manager = reinterpret_cast<DeviceManager*>(dwInstance);
        if (manager)
        {
            manager->HandleAudioData((LPWAVEHDR)dwParam1);
        }
    }
}

void DeviceManager::HandleAudioData(LPWAVEHDR lpWaveHdr)
{
    // Skip processing if we're shutting down
    if (m_isShuttingDown)
    {
        LogMessage(L"\nSkipping audio processing - shutdown in progress");
        return;
    }

    int bufferIndex = static_cast<int>(lpWaveHdr->dwUser);
    if (bufferIndex >= 0 && bufferIndex < m_audioBuffers.size())
    {
        m_audioBuffers[bufferIndex].inUse = true;
    }

    if (m_hWaveOut && lpWaveHdr->dwBytesRecorded > 0)
    {
        wchar_t debugMsg[256];
        swprintf_s(debugMsg, L"\nReceived audio data: %d bytes, buffer %d", 
                  lpWaveHdr->dwBytesRecorded, static_cast<DWORD>(lpWaveHdr->dwUser));
        LogMessage(debugMsg);

        // Get the corresponding output buffer
        LPWAVEHDR outHdr = &m_audioBuffers[lpWaveHdr->dwUser].outHeader;
        
        // Copy the data to the output buffer
        memcpy(outHdr->lpData, lpWaveHdr->lpData, lpWaveHdr->dwBytesRecorded);
        outHdr->dwBufferLength = lpWaveHdr->dwBytesRecorded;
        
        // Write the audio data to the output device
        MMRESULT result = waveOutWrite(m_hWaveOut, outHdr, sizeof(WAVEHDR));
        
        if (result == MMSYSERR_NOERROR)
        {
            LogMessage(L"\nWrote data to output device");
        }
        else
        {
            swprintf_s(debugMsg, L"\nFailed to write to output device, error: %d", result);
            LogMessage(debugMsg);
        }

        // Only requeue if we're not shutting down
        if (!m_isShuttingDown)
        {
            // Requeue the input buffer
            result = waveInAddBuffer(m_hWaveIn, lpWaveHdr, sizeof(WAVEHDR));
            if (result != MMSYSERR_NOERROR)
            {
                swprintf_s(debugMsg, L"\nFailed to requeue input buffer, error: %d", result);
                LogMessage(debugMsg);
            }
            else
            {
                LogMessage(L"\nSuccessfully requeued input buffer");
            }
        }
        
        // Rotate to next buffer
        m_currentBuffer = (m_currentBuffer + 1) % NUM_BUFFERS;
    }
    else
    {
        if (!m_hWaveOut)
        {
            LogMessage(L"\nOutput device handle is null");
        }
        if (lpWaveHdr->dwBytesRecorded == 0)
        {
            LogMessage(L"\nNo bytes recorded in buffer");
            
            // Only requeue if we're not shutting down
            if (!m_isShuttingDown)
            {
                MMRESULT result = waveInAddBuffer(m_hWaveIn, lpWaveHdr, sizeof(WAVEHDR));
                if (result != MMSYSERR_NOERROR)
                {
                    LogMessage(L"\nFailed to requeue empty buffer");
                }
            }
        }
    }

    if (bufferIndex >= 0 && bufferIndex < m_audioBuffers.size())
    {
        m_audioBuffers[bufferIndex].inUse = false;
    }
}

void DeviceManager::DisconnectAudioDevices()
{
    LogMessage(L"\nDisconnecting audio devices...");
    
    // Set shutdown flag to prevent new buffer queuing
    m_isShuttingDown = true;
    
    if (m_hWaveIn)
    {
        LogMessage(L"\nStopping input device...");
        waveInStop(m_hWaveIn);
        
        // Wait for any in-flight buffers to complete
        LogMessage(L"\nWaiting for buffers to complete...");
        bool buffersInUse;
        do {
            buffersInUse = false;
            for (const auto& buffer : m_audioBuffers)
            {
                if (buffer.inUse)
                {
                    buffersInUse = true;
                    Sleep(1);
                    break;
                }
            }
        } while (buffersInUse);
        
        LogMessage(L"\nResetting devices...");
        waveInReset(m_hWaveIn);
        if (m_hWaveOut)
        {
            waveOutReset(m_hWaveOut);
        }
        
        LogMessage(L"\nUnpreparing buffers...");
        for (auto& buffer : m_audioBuffers)
        {
            if (m_hWaveIn)
            {
                waveInUnprepareHeader(m_hWaveIn, &buffer.inHeader, sizeof(WAVEHDR));
            }
            if (m_hWaveOut)
            {
                waveOutUnprepareHeader(m_hWaveOut, &buffer.outHeader, sizeof(WAVEHDR));
            }
        }
        
        LogMessage(L"\nClosing devices...");
        waveInClose(m_hWaveIn);
        m_hWaveIn = nullptr;
        
        if (m_hWaveOut)
        {
            waveOutClose(m_hWaveOut);
            m_hWaveOut = nullptr;
        }
    }
    else if (m_hWaveOut)
    {
        waveOutReset(m_hWaveOut);
        waveOutClose(m_hWaveOut);
        m_hWaveOut = nullptr;
    }
    
    m_audioConnected = false;
    m_audioBuffers.clear();
    LogMessage(L"\nAudio devices disconnected");
}

std::vector<MidiDeviceInfo> DeviceManager::EnumerateMidiInputDevices() const
{
    std::vector<MidiDeviceInfo> devices;
    
    // Add "No Device" option
    MidiDeviceInfo noDevice;
    noDevice.deviceId = MIDI_MAPPER;
    noDevice.name = L"No Device";
    noDevice.isInput = true;
    devices.push_back(noDevice);

    // Enumerate MIDI input devices
    UINT numDevices = midiInGetNumDevs();
    for (UINT i = 0; i < numDevices; i++)
    {
        MIDIINCAPSW caps;
        if (midiInGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
        {
            MidiDeviceInfo device;
            device.deviceId = i;
            device.name = caps.szPname;
            device.isInput = true;
            devices.push_back(device);
        }
    }

    return devices;
}

std::vector<MidiDeviceInfo> DeviceManager::EnumerateMidiOutputDevices() const
{
    std::vector<MidiDeviceInfo> devices;
    
    // Add "No Device" option
    MidiDeviceInfo noDevice;
    noDevice.deviceId = MIDI_MAPPER;
    noDevice.name = L"No Device";
    noDevice.isInput = false;
    devices.push_back(noDevice);

    // Enumerate MIDI output devices
    UINT numDevices = midiOutGetNumDevs();
    for (UINT i = 0; i < numDevices; i++)
    {
        MIDIOUTCAPSW caps;
        if (midiOutGetDevCapsW(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
        {
            MidiDeviceInfo device;
            device.deviceId = i;
            device.name = caps.szPname;
            device.isInput = false;
            devices.push_back(device);
        }
    }

    return devices;
}

bool DeviceManager::ConnectMidiInputToOutput(const MidiDeviceInfo& input, const MidiDeviceInfo& output)
{
    if (m_midiConnected)
    {
        DisconnectMidiDevices();
    }

    if (input.deviceId == MIDI_MAPPER || output.deviceId == MIDI_MAPPER)
    {
        return false;
    }

    // Open MIDI input device with callback
    MMRESULT result = midiInOpen(&m_hMidiIn, input.deviceId, (DWORD_PTR)MidiInProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR)
    {
        return false;
    }

    // Open MIDI output device
    result = midiOutOpen(&m_hMidiOut, output.deviceId, 0, 0, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR)
    {
        midiInClose(m_hMidiIn);
        m_hMidiIn = nullptr;
        return false;
    }

    // Start recording MIDI input
    result = midiInStart(m_hMidiIn);
    if (result != MMSYSERR_NOERROR)
    {
        midiInClose(m_hMidiIn);
        midiOutClose(m_hMidiOut);
        m_hMidiIn = nullptr;
        m_hMidiOut = nullptr;
        return false;
    }

    m_midiConnected = true;
    return true;
}

void CALLBACK DeviceManager::MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    if (wMsg == MIM_DATA)
    {
        DeviceManager* manager = reinterpret_cast<DeviceManager*>(dwInstance);
        if (manager)
        {
            manager->HandleMidiMessage(dwParam1, dwParam2);
        }
    }
}

void DeviceManager::HandleMidiMessage(DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    if (m_hMidiOut)
    {
        // Extract MIDI message components
        BYTE status = (BYTE)(dwParam1 & 0xFF);
        BYTE data1 = (BYTE)((dwParam1 >> 8) & 0xFF);
        BYTE data2 = (BYTE)((dwParam1 >> 16) & 0xFF);
        DWORD timestamp = (DWORD)dwParam2;

        // Forward the message to the output device
        midiOutShortMsg(m_hMidiOut, static_cast<DWORD>(dwParam1));
    }
}

void DeviceManager::DisconnectMidiDevices()
{
    if (m_hMidiIn)
    {
        midiInStop(m_hMidiIn);
        midiInClose(m_hMidiIn);
        m_hMidiIn = nullptr;
    }
    if (m_hMidiOut)
    {
        midiOutClose(m_hMidiOut);
        m_hMidiOut = nullptr;
    }
    m_midiConnected = false;
}

std::wstring DeviceManager::GetDeviceName(UINT deviceId, bool isInput)
{
    if (deviceId == WAVE_MAPPER || deviceId == MIDI_MAPPER)
    {
        return L"No Device";
    }

    if (isInput)
    {
        WAVEINCAPSW caps;
        if (waveInGetDevCapsW(deviceId, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
        {
            return caps.szPname;
        }
    }
    else
    {
        WAVEOUTCAPSW caps;
        if (waveOutGetDevCapsW(deviceId, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
        {
            return caps.szPname;
        }
    }

    return L"Unknown Device";
}

bool DeviceManager::IsDeviceAvailable(UINT deviceId, bool isInput)
{
    if (deviceId == WAVE_MAPPER || deviceId == MIDI_MAPPER)
    {
        return true;
    }

    if (isInput)
    {
        WAVEINCAPSW caps;
        return waveInGetDevCapsW(deviceId, &caps, sizeof(caps)) == MMSYSERR_NOERROR;
    }
    else
    {
        WAVEOUTCAPSW caps;
        return waveOutGetDevCapsW(deviceId, &caps, sizeof(caps)) == MMSYSERR_NOERROR;
    }
} 