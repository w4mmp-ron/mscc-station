namespace MSCC.Core.Protocol;

using System.Collections.Generic;
using System.Reflection;

/// <summary>
/// Centralized definition of protocol opcodes used to communicate with the backend servers.
/// These were originally scattered across the old WinForms codebase.
/// </summary>
public static class Opcodes
{
    private static readonly Dictionary<byte, string> _opcodeNames = new();

    static Opcodes()
    {
        // Build name lookup from the public const byte fields (handles duplicates by taking first encountered)
        var fields = typeof(Opcodes).GetFields(BindingFlags.Public | BindingFlags.Static);
        foreach (var field in fields)
        {
            if (field.FieldType == typeof(byte) && field.IsLiteral)
            {
                byte value = (byte)field.GetRawConstantValue()!;
                if (!_opcodeNames.ContainsKey(value))
                {
                    _opcodeNames[value] = field.Name;
                }
            }
        }
    }

    /// <summary>
    /// Returns a friendly name for the opcode (e.g. "CMD_SET_KEEP_ALIVE") or "0xXX" if unknown.
    /// Useful for logging received packets from ms-sdr so we know what is being sent.
    /// </summary>
    public static string GetName(byte opcode)
    {
        return _opcodeNames.TryGetValue(opcode, out var name) ? name : $"0x{opcode:X2}";
    }

    // System / Lifecycle
    public const byte CMD_GUI_RUNNING              = 0xFE;
    public const byte CMD_CHECK_GUI_STATUS         = 0xFE;  // same opcode, sent with data=1 to signal ready, triggers ms-sdr startup responses including CMD_GET_SET_STARTUP_BAND
    public const byte CMD_SET_STOP                 = 0xFF;
    public const byte CMD_SET_KEEP_ALIVE           = 0xF4;
    public const byte CMD_SET_RELOAD_FILE          = 0xA5;

    // Frequency & Mode
    public const byte CMD_SET_MAIN_FREQ            = 0xB6;
    public const byte CMD_SET_MAIN_MODE            = 0xB7;
    public const byte CMD_GET_FREQ_INIT            = 0xB0;
    public const byte CMD_GET_MODE_INIT            = 0xB8;
    public const byte CMD_GET_BAND_INIT            = 0xB9;
    public const byte CMD_GET_SET_LAST_USED_FREQ   = 0xD7;
    public const byte CMD_GET_SET_LAST_USED_MODE   = 0xD8;
    public const byte CMD_GET_SET_LAST_USED_BAND   = 0xD9;
    public const byte CMD_SET_DISPLAY_FREQ         = 0xBB;  // used by ms-sdr for startup frequency (Gui_send_param(CMD_SET_DISPLAY_FREQ, G_tune_freq)) when favorites not yet implemented

    /// <summary>
    /// Select active VFO on ms-sdr. Payload: VFO_A (0) or VFO_B (1).
    /// Must be sent before frequency/mode when the user switches VFOs (prep for split operating).
    /// </summary>
    public const byte CMD_SET_VFO                  = 0xF2;
    public const byte VFO_A                        = 0;
    public const byte VFO_B                        = 1;

    // Transmit
    public const byte CMD_SET_TX_ON                = 0xBA;
    public const byte CMD_SET_RIG_TUNE             = 0xA6;
    public const byte CMD_SET_TX_SET_BY_SERVER     = 0xBC;
    /// <summary>
    /// PA / QRP vs QRO (Full). Payload: QRP_MODE (0) or QRO_MODE (1). Not 0xF2 — that is CMD_SET_VFO only.
    /// </summary>
    public const byte CMD_SET_PA_BYPASS            = 0xF7;
    public const byte QRP_MODE                     = 0; // PA bypass off / QRP
    public const byte QRO_MODE                     = 1; // PA path / full (non-QRP)

    // Power & Calibration
    public const byte CMD_SET_BAND_POWER_BAND      = 0xA1;
    public const byte CMD_SET_BAND_POWER_POWER     = 0xA2;
    public const byte CMD_SET_BAND_POWER_DEFAULTS  = 0xAA;
    public const byte CMD_GET_BAND_POWER           = 0xB4;
    /// <summary>Power-cal TX carrier on/off (original Power_Calibration_Controls.CMD_CALIBRATION_TUNE).</summary>
    public const byte CMD_CALIBRATION_TUNE         = 0xAC;
    public const byte CMD_SET_COMMIT_POWER_VALUES  = 0xAD;
    public const byte CMD_CALIBRATION_MASTER_RESET = 0xAB;

    // Frequency Calibration (from original Frequency_Calibration_controls)
    public const byte CMD_SET_CALIBRATION_FINISHED = 0x62;
    public const byte CMD_SET_CAL_LOOSE            = 0x63;
    public const byte CMD_SET_CAL_SET_COARSE       = 0xC3;
    public const byte CMD_SET_FORCE_CALIBRATION    = 0xC5;
    public const byte CMD_SET_CAL_SET_FINE         = 0xAE;
    public const byte CMD_SET_CAL_RESET            = 0x68;
    public const byte CMD_SET_CAL_MODE             = 0x69; // 0 = coarse (±250 Hz), 1 = fine (±50 Hz)
    public const byte CMD_SET_FREQ_CAL_CHECK       = 0x8C;
    public const byte CMD_START_CALIBRATE          = 0xA7;
    public const byte CMD_SET_STANDARD_CARRIER     = 0xAF;
    public const byte CMD_SET_CALIBRATIION_PROGRESS = 0x6A; // as in original (note spelling)
    public const byte CMD_GET_SET_CAL_FREQ_DELTA   = 0x6B;

    // Audio Devices
    public const byte CMD_GET_SET_SPEAKER_DEVICE   = 0xEA;
    public const byte CMD_GET_SET_MIC_DEVICE       = 0xEB;
    public const byte CMD_SET_PHONES_VOLUME_LEVEL  = 0x97;
    public const byte CMD_SET_PHONES_MIC_GAIN_LEVEL = 0x98;
    public const byte CMD_SET_DIGITAL_VOLUME_LEVEL = 0x99;
    public const byte CMD_SET_DIGITAL_MIC_GAIN_LEVEL = 0x9A;
    public const byte CMD_SET_AUDIO_DEVICE = 0x9B;
    public const byte DIGITAL_SOUND_DEVICE = 0;
    public const byte PHONES_SOUND_DEVICE = 1;

    // Status & Control
    public const byte CMD_SET_HDSDR_STATUS         = 0xF0;
    public const byte CMD_GET_HDSDR_STATUS         = 0xF1;
    public const byte CMD_GET_SET_MSSDR_STATUS     = 0xF5;
    public const byte CMD_GET_SET_STARTUP_BAND     = 0xF6;
    public const byte CMD_SET_TRANSVERTER          = 0xA9;

    // Panadapter / Spectrum (from backend)
    public const byte CMD_GET_SET_PANADAPTER = 0xD5;
    /// <summary>
    /// MSCC-Net9: pan resolution index 0=800, 1=1600, 2=3200 bins (forwards as legacy REFRESH opcode).
    /// Legacy refresh values 3–10 adjust SDRcore block count only.
    /// </summary>
    public const byte CMD_GET_SET_PANADAPTER_REFRESH = 0x5F;

    // Incoming reports from backend (server -> GUI) - values from original protocol
    public const byte CMD_MODE_SET_BY_SERVER = 0xA8;
    public const byte CMD_GET_SET_SMETER = 0xD4;
    /// <summary>ALC meter value report from server (not the ALC button).</summary>
    public const byte CMD_SET_ALC = 0x4F;
    /// <summary>ALC on/off (Rx/Tx ALC button). Payload: 1=on, 0=off. Server handles as multiplier enable.</summary>
    public const byte CMD_SET_ALC_MULTIPLIER = 0x23;
    public const byte CMD_GET_AMP_POWER = 0x05;
    /// <summary>Amplifier cal band init (original Amplifier_Power_Controls.CMD_SET_AMPLIFIER_INITIALIZE).</summary>
    public const byte CMD_SET_AMPLIFIER_INITIALIZE = 0xF9;
    /// <summary>Amplifier power level for cal (original CMD_SET_AMPLIFIER_POWER); band select sends 100.</summary>
    public const byte CMD_SET_AMPLIFIER_POWER = 0xFA;
    public const byte CMD_GET_AMPLIFIER_POWER = 0xFB;
    /// <summary>Amplifier cal factory reset (original CMD_SET_AMPLIFIER_CALIBRATION_RESET).</summary>
    public const byte CMD_SET_AMPLIFIER_CALIBRATION_RESET = 0x10;
    /// <summary>Potentia calibration step (original CMD_SET_POTENTIA_CALIBRATION); int32 payload in original.</summary>
    public const byte CMD_SET_POTENTIA_CALIBRATION = 0x08;
    // Panadapter/spectrum freq/mode reports (from original Spectrum_Panadapter_Controls / Waterfall)
    public const byte CMD_SET_SPECTRUM_WATERFALL_FREQ = 0x91;
    public const byte CMD_SET_SPECTRUM_WATERFALL_MODE = 0x92;

    // Filter / CW (from Filter_control)
    public const byte CMD_SET_BW_LOCUT = 0xD0;
    public const byte CMD_SET_BW_HICUT = 0xD1;
    public const byte CMD_SET_CW_PITCH = 0xD2;
    public const byte CMD_SET_TX_HICUT = 0xD3;
    public const byte CMD_SET_STEP_VALUE = 0xCE;
    public const byte CMD_SET_CW_BW = 0xDB;
    public const byte CMD_SET_CW_KEYER_MODE = 0x71;
    public const byte CMD_SET_CW_SPACING = 0x75;
    public const byte CMD_SET_CW_PADDLE = 0x73;
    public const byte CMD_SET_CW_WEIGHT = 0x77;
    public const byte CMD_SET_CW_TX_HOLD = 0x7A;
    public const byte CMD_SET_CW_WPM = 0x7B;
    public const byte CMD_SET_CW_QSK = 0x72;
    public const byte CMD_SET_CW_MODE = 0x70;
    /// <summary>
    /// Keyer CQ/memory (PIC 0x9C). Int payload low byte:
    /// 0=play, 1=store begin, 2=store end, 3=select slot (next param 0..3),
    /// 0x20-0x7E=append ASCII. Sticky slot; default 0. Max 48 chars/slot, 4 slots.
    /// </summary>
    public const byte CMD_SET_KEYER_MEMORY = 0x9C;
    public const int KEYER_MEM_PLAY = 0;
    public const int KEYER_MEM_STORE_BEGIN = 1;
    public const int KEYER_MEM_STORE_END = 2;
    public const int KEYER_MEM_SELECT = 3;
    public const int KEYER_MEM_MAX_CHARS = 48;
    public const int KEYER_MEM_SLOT_COUNT = 4;
    /// <summary>Farnsworth text/overall WPM for memory play gaps only (0=off, 5–60).</summary>
    public const byte SET_MEM_TEXT_WPM = 0x76;

    // Default filters (from Filter_control, for Rx/Tx tab)
    public const byte CMD_SET_BW_LOCUT_DEFAULT = 0xDC;
    public const byte CMD_SET_BW_HICUT_DEFAULT = 0xDD;
    public const byte CMD_SET_CW_BW_DEFAULT = 0xDE;

    // RIT (from Rit_Controls)
    public const byte CMD_SET_RIT_FREQ = 0x89;
    public const byte CMD_SET_RIT_STATUS = 0x8A;

    // TX Power modes (from Power_Controls)
    public const byte CMD_SET_MAIN_POWER = 0xE2;
    public const byte CMD_SET_AM_POWER = 0xE3;
    public const byte CMD_SET_CW_POWER = 0xE4;
    public const byte CMD_SET_TUNE_POWER = 0xE9;

    // Noise blanker (bidirectional — original NB_Controls)
    public const byte CMD_GET_SET_NB_ENABLE = 0x80;
    public const byte CMD_GET_SET_NB_PULSE_WIDTH = 0x81;
    public const byte CMD_GET_SET_NB_THRESHOLD = 0x82;

    /// <summary>
    /// Noise reduction (NR button / level). Server: usbavrcmd.h / commands.h CMD_SET_NR.
    /// Payload 0 = OFF; non-zero = ON with that level (matches SDRcore-recv).
    /// </summary>
    public const byte CMD_SET_NR = 0xA3;

    /// <summary>
    /// Auto notch on/off (AN button). Server: CMD_GET_SET_AUTO_NOTCH.
    /// Payload 0 = OFF; non-zero = ON (sdrcore-recv anstate.enabled). No level parameter.
    /// </summary>
    public const byte CMD_GET_SET_AUTO_NOTCH = 0x8E;

    // AGC (from AGC_ALC_Notch_Controls)
    public const byte CMD_GET_SET_AGC = 0x87;
    public const byte CMD_SET_AGC_FAST_LEVEL = 0xCB;

    // Audio / Volume (from Aud/Sys and Volume_Controls)
    public const byte CMD_SET_MIC_GAIN = 0xE0;
    public const byte CMD_SET_DIGITAL_MIC_GAIN = 0x93;
    public const byte CMD_SET_VOLUME_ATTN = 0xE1;
    public const byte CMD_SET_DIGITAL_VOLUME_ATTN = 0x96;
    public const byte CMD_SET_SPEAKER_VOLUME = 0xE5;
    public const byte CMD_SET_MIC_VOLUME = 0xE6;
    public const byte CMD_SET_VOLUME_BY_SERVER = 0x0E;
    public const byte CMD_SET_COMPRESSION_STATE = 0xEE;
    public const byte CMD_SET_COMPRESSION_LEVEL = 0xEF;
    public const byte CMD_SET_MONITOR = 0x0D;

    // System / Transverter / Relay
    // Relay is often local FTDI (not CMD_SET_VFO 0xF2). Placeholder until FTDI path is ported.
    public const byte CMD_SET_RELAY_BOARD = 0x00;

    // Audio mode (P=phones/operator, D=digital) from original
    public const byte CMD_SET_CONFIGURATION = 0x24;

    // Serial/Comm port settings (from original guiCode)
    public const byte CMD_GET_SET_COMM_BAUD_RATE = 0x41;
    public const byte CMD_GET_SET_COMM_PARITY = 0x42;
    public const byte CMD_GET_SET_COMM_DATA_BITS = 0x43;
    public const byte CMD_GET_SET_COMM_STOP_BITS = 0x44;
    public const byte CMD_GET_SET_COMM_NAME_INDEX = 0x46;
    public const byte CMD_GET_SET_COMM_PORT_PINS = 0x48;

    // Temps and versions (reports)
    public const byte CMD_GET_TRANSCEIVER_TEMP = 0xBF;
    // CMD_RPI_SET_TEMPERATURE (0x12) ignored per user - not valid for this work
    // public const byte CMD_RPI_SET_TEMPERATURE = 0x12;
    public const byte CMD_GET_SET_FIRMWARE_VERSION = 0xB2;
    public const byte CMD_GET_SET_MSSDR_VERSION = 0xB3;
    public const byte CMD_GET_SET_SDRCORE_RECV_VERSION = 0xB5;
    public const byte CMD_GET_SET_SDRCORE_TRANS_VERSION = 0xBD;
    public const byte CMD_GET_OPTIONS_STATUS = 0xBE;
    public const byte CMD_SET_SOLIDUS_STATUS = 0x0E;

    // TX / RX I/Q balance calibration (original IQ_Controls)
    public const byte CMD_SET_IQ_OFFSET = 0x52;
    public const byte IQ_CALIBRATION_TUNE = 0x54;
    public const byte IQ_CALIBRATION_RX_TX = 0x55;
    public const byte IQ_OPERATION_COMPLETE = 0x56;
    public const byte CMD_SET_COMMIT_IQ = 0x57;
    public const byte CMD_SET_IQ_BAND = 0x58;
    public const byte CMD_GET_IQ_VALUE = 0x8B;
    public const byte CMD_SET_IQ_ALL_BANDS = 0x8D;
    public const byte CMD_REPORT_IMAGE_VALUE = 0xC6;
    public const byte CMD_START_STOP_IMAGE_VALUE = 0xC7;
    /// <summary>Payload for IQ_CALIBRATION_RX_TX: 0 = RX IQ, 1 = TX IQ.</summary>
    public const byte RX_IQBD = 0;
    public const byte TX_IQBD = 1;

    // Extended commands (0x0B is the wrapper; sub-opcode in first byte of payload)
    public const byte CMD_SET_EXTENDED_COMMAND = 0x0B;
    /// <summary>Extended sub: enable/disable IQBD image monitor stream (payload byte 1 = 0/1).</summary>
    public const byte EXT_CMD_SET_IQBD_MONITOR = 0x09;
    /// <summary>Extended sub: IQBD residual/image sample (Int16 at payload offset 1 after sub).</summary>
    public const byte EXT_CMD_SET_IQBD_DATA = 0x0A;

    // TODO: add more as reverse-engineered (status, extended power, temps, drift, antenna, etc.)
}
