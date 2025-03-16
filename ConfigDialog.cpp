#include "ConfigDialog.h"
#include <commctrl.h>
#include <windowsx.h>

ConfigDialog::ConfigDialog(HWND hParent)
    : m_hParent(hParent)
{
}

ConfigDialog::~ConfigDialog()
{
}

bool ConfigDialog::Show()
{
    return DialogBoxParamW(GetModuleHandle(nullptr),
        MAKEINTRESOURCEW(IDD_CONFIG_DIALOG),
        m_hParent,
        DialogProc,
        reinterpret_cast<LPARAM>(this)) == IDOK;
}

INT_PTR CALLBACK ConfigDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ConfigDialog* dialog = nullptr;
    if (uMsg == WM_INITDIALOG)
    {
        dialog = reinterpret_cast<ConfigDialog*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(dialog));
    }
    else
    {
        dialog = reinterpret_cast<ConfigDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (dialog)
    {
        return dialog->HandleMessage(hwnd, uMsg, wParam, lParam);
    }

    return FALSE;
}

INT_PTR ConfigDialog::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            InitializeControls(hwnd);
            UpdateDeviceLists(hwnd);
            return TRUE;

        case WM_CLOSE:
            OnCancel(hwnd);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_TEST_AUDIO_CHECK:
                    OnTestAudioChanged(hwnd, IsDlgButtonChecked(hwnd, IDC_TEST_AUDIO_CHECK) == BST_CHECKED);
                    return TRUE;

                case IDC_TEST_MIDI_CHECK:
                    OnTestMidiChanged(hwnd, IsDlgButtonChecked(hwnd, IDC_TEST_MIDI_CHECK) == BST_CHECKED);
                    return TRUE;

                case IDC_OK_BUTTON:
                    OnOK(hwnd);
                    return TRUE;

                case IDC_CANCEL_BUTTON:
                    OnCancel(hwnd);
                    return TRUE;
            }
            break;
    }

    return FALSE;
}

void ConfigDialog::InitializeControls(HWND hwnd)
{
    // Center the dialog
    RECT rc;
    GetWindowRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(hwnd, nullptr,
        (screenWidth - width) / 2,
        (screenHeight - height) / 2,
        0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void ConfigDialog::UpdateDeviceLists(HWND hwnd)
{
    // Get device lists
    m_audioInputDevices = m_deviceManager.EnumerateAudioInputDevices();
    m_audioOutputDevices = m_deviceManager.EnumerateAudioOutputDevices();
    m_midiInputDevices = m_deviceManager.EnumerateMidiInputDevices();
    m_midiOutputDevices = m_deviceManager.EnumerateMidiOutputDevices();

    // Update audio input combo
    HWND hAudioInputCombo = GetDlgItem(hwnd, IDC_AUDIO_INPUT_COMBO);
    SendMessageW(hAudioInputCombo, CB_RESETCONTENT, 0, 0);
    for (const auto& device : m_audioInputDevices)
    {
        SendMessageW(hAudioInputCombo, CB_ADDSTRING, 0, (LPARAM)device.name.c_str());
    }
    SendMessageW(hAudioInputCombo, CB_SETCURSEL, 0, 0);

    // Update audio output combo
    HWND hAudioOutputCombo = GetDlgItem(hwnd, IDC_AUDIO_OUTPUT_COMBO);
    SendMessageW(hAudioOutputCombo, CB_RESETCONTENT, 0, 0);
    for (const auto& device : m_audioOutputDevices)
    {
        SendMessageW(hAudioOutputCombo, CB_ADDSTRING, 0, (LPARAM)device.name.c_str());
    }
    SendMessageW(hAudioOutputCombo, CB_SETCURSEL, 0, 0);

    // Update MIDI input combo
    HWND hMidiInputCombo = GetDlgItem(hwnd, IDC_MIDI_INPUT_COMBO);
    SendMessageW(hMidiInputCombo, CB_RESETCONTENT, 0, 0);
    for (const auto& device : m_midiInputDevices)
    {
        SendMessageW(hMidiInputCombo, CB_ADDSTRING, 0, (LPARAM)device.name.c_str());
    }
    SendMessageW(hMidiInputCombo, CB_SETCURSEL, 0, 0);

    // Update MIDI output combo
    HWND hMidiOutputCombo = GetDlgItem(hwnd, IDC_MIDI_OUTPUT_COMBO);
    SendMessageW(hMidiOutputCombo, CB_RESETCONTENT, 0, 0);
    for (const auto& device : m_midiOutputDevices)
    {
        SendMessageW(hMidiOutputCombo, CB_ADDSTRING, 0, (LPARAM)device.name.c_str());
    }
    SendMessageW(hMidiOutputCombo, CB_SETCURSEL, 0, 0);
}

void ConfigDialog::OnTestAudioChanged(HWND hwnd, bool checked)
{
    if (checked)
    {
        int inputIndex = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_AUDIO_INPUT_COMBO));
        int outputIndex = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_AUDIO_OUTPUT_COMBO));

        if (inputIndex >= 0 && outputIndex >= 0 &&
            inputIndex < static_cast<int>(m_audioInputDevices.size()) &&
            outputIndex < static_cast<int>(m_audioOutputDevices.size()))
        {
            const auto& input = m_audioInputDevices[inputIndex];
            const auto& output = m_audioOutputDevices[outputIndex];

            if (!m_deviceManager.ConnectAudioInputToOutput(input, output))
            {
                MessageBoxW(hwnd, L"Failed to connect audio devices", L"Error", MB_ICONERROR);
                CheckDlgButton(hwnd, IDC_TEST_AUDIO_CHECK, BST_UNCHECKED);
                return;
            }
        }
    }
    else
    {
        m_deviceManager.DisconnectAudioDevices();
    }
}

void ConfigDialog::OnTestMidiChanged(HWND hwnd, bool checked)
{
    if (checked)
    {
        int inputIndex = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_MIDI_INPUT_COMBO));
        int outputIndex = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_MIDI_OUTPUT_COMBO));

        if (inputIndex >= 0 && outputIndex >= 0 &&
            inputIndex < static_cast<int>(m_midiInputDevices.size()) &&
            outputIndex < static_cast<int>(m_midiOutputDevices.size()))
        {
            const auto& input = m_midiInputDevices[inputIndex];
            const auto& output = m_midiOutputDevices[outputIndex];

            if (!m_deviceManager.ConnectMidiInputToOutput(input, output))
            {
                MessageBoxW(hwnd, L"Failed to connect MIDI devices", L"Error", MB_ICONERROR);
                CheckDlgButton(hwnd, IDC_TEST_MIDI_CHECK, BST_UNCHECKED);
                return;
            }
        }
    }
    else
    {
        m_deviceManager.DisconnectMidiDevices();
    }
}

void ConfigDialog::OnOK(HWND hwnd)
{
    // Disconnect any test connections
    m_deviceManager.DisconnectAudioDevices();
    m_deviceManager.DisconnectMidiDevices();

    EndDialog(hwnd, IDOK);
}

void ConfigDialog::OnCancel(HWND hwnd)
{
    // Disconnect any test connections
    m_deviceManager.DisconnectAudioDevices();
    m_deviceManager.DisconnectMidiDevices();

    EndDialog(hwnd, IDCANCEL);
} 