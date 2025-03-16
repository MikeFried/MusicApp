#pragma once

#include <windows.h>
#include "DeviceManager.h"

// Dialog control IDs
#define IDD_CONFIG_DIALOG 101
#define IDC_AUDIO_INPUT_COMBO 1001
#define IDC_AUDIO_OUTPUT_COMBO 1002
#define IDC_MIDI_INPUT_COMBO 1003
#define IDC_MIDI_OUTPUT_COMBO 1004
#define IDC_TEST_AUDIO_CHECK 1005
#define IDC_TEST_MIDI_CHECK 1006
#define IDC_OK_BUTTON 1007
#define IDC_CANCEL_BUTTON 1008

class ConfigDialog {
public:
    ConfigDialog(HWND hParent);
    ~ConfigDialog();

    bool Show();

private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void InitializeControls(HWND hwnd);
    void UpdateDeviceLists(HWND hwnd);
    void OnTestAudioChanged(HWND hwnd, bool checked);
    void OnTestMidiChanged(HWND hwnd, bool checked);
    void OnOK(HWND hwnd);
    void OnCancel(HWND hwnd);

    HWND m_hParent;
    DeviceManager m_deviceManager;
    std::vector<AudioDeviceInfo> m_audioInputDevices;
    std::vector<AudioDeviceInfo> m_audioOutputDevices;
    std::vector<MidiDeviceInfo> m_midiInputDevices;
    std::vector<MidiDeviceInfo> m_midiOutputDevices;
}; 