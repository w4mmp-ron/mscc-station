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

    /// <summary>Enable/disable TX/RX split (ms-sdr G_Split). Payload 0/1.</summary>
    public const byte CMD_SET_SPLIT                = 0x31;
    /// <summary>Split RX frequency (Hz, int32 LE) — listen freq when split on.</summary>
    public const byte CMD_SET_SPLIT_RX_FREQ        = 0x34;
    /// <summary>Split TX frequency (Hz, int32 LE) — PTT freq when split on.</summary>
    public const byte CMD_SET_SPLIT_TX_FREQ        = 0x39;
    public const byte CMD_GET_FREQ_INIT            = 0xB0;
    public const byte CMD_GET_MODE_INIT            = 0xB8;
    public const byte CMD_GET_BAND_INIT            = 0xB9;
    // 0xD7-0xD9 (CMD_GET_SET_LAST_USED_FREQ / _MODE / _BAND) reclaimed 2026-11-10: the server
    // last-used band stack was removed (ms-sdr 3.199); the client keeps its own last-used.
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
    /// <summary>Park live iq.ini / power_cal.ini / amplifier_cal.ini for the current radio line (ms-sdr). Payload 1.</summary>
    public const byte CMD_SET_PARK_CAL_SETTINGS    = 0x29;
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
    /// <summary>
    /// Client to server: stop an AUTO or CHECK sweep. Does not change the stored PPM.
    /// Server to client, 16-bit: 1 = abort drain done, 2 = start refused while the drain is open.
    /// </summary>
    public const byte CMD_SET_CAL_ABORT            = 0x1F;
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
    /// <summary>IPv4 destination for phones MSA1 (4 bytes network order). Recv only.</summary>
    public const byte CMD_SET_REMOTE_RX_HOST = 0x25;
    /// <summary>uint32 LE: port[15:0] | enable&lt;&lt;16 | monitor-at-radio&lt;&lt;17. Recv only.</summary>
    public const byte CMD_SET_REMOTE_RX_CTRL = 0x28;
    public const uint RemoteRxCtrlEnable = 1u << 16;
    public const uint RemoteRxCtrlMonitor = 1u << 17;
    public const byte DIGITAL_SOUND_DEVICE = 0;
    public const byte PHONES_SOUND_DEVICE = 1;
    /// <summary>Operator mic via MSA1 UDP (client → sdrcore-trans). Phones levels apply.</summary>
    public const byte REMOTE_SOUND_DEVICE = 2;
    /// <summary>R-Digital: recv taps VirtualA path + MSA1; trans uses MSA1 as digital mic.</summary>
    public const byte REMOTE_DIGITAL_SOUND_DEVICE = 3;

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

    /// <summary>Server spectrum smoothing (ms-sdr PANADAPTER_AVERAGE → recv). 0 = 2-frame average; N = running average, time constant N × 25 ms.</summary>
    public const byte CMD_GET_SET_PANADAPTER_SMOOTHING = 0xDA;

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
    /// <summary>Farnsworth on the paddles (buffered PIC keyer), 0 = off, 1 = on. ms-sdr saves it in
    /// cw.ini (CW_Paddle_Farnsworth=), sends it to the radio and reports it at connect.</summary>
    public const byte SET_PADDLE_FARNSWORTH = 0x9D;

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
    /// <summary>NFM drive power percent (sdrcore-trans / ms-sdr).</summary>
    public const byte CMD_SET_FM_POWER = 0x9E;

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
    /// <summary>Heat sink temperature (ms-sdr 3.183+ Pi / 3.191+ Windows, STM32 radio only; the
    /// PSoC does not send it). Int32 °C; -99 = thermistor open / unplugged, 999 = shorted.</summary>
    public const byte CMD_GET_POTENTIA_TEMPERATURE = 0x06;
    public const int HEAT_SINK_OPEN = -99;
    public const int HEAT_SINK_SHORT = 999;
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
    /// <summary>TX I/Q phase (Int32 steps of 0.05 deg) to ms-sdr -> trans (2026-10-30).</summary>
    public const byte CMD_SET_IQ_PHASE = 0x2A;
    /// <summary>TX I/Q phase in use, from trans via ms-sdr (Int32 steps of 0.05 deg).</summary>
    public const byte CMD_GET_IQ_PHASE = 0x2B;
    /// <summary>
    /// Radio calibration status from ms-sdr (3.189+), one message per kind. Int32 = kind (1 RX IQ,
    /// 2 TX IQ, 3 power) | flags &lt;&lt; 8 (bit 0 = radio has a calibration store) | band mask &lt;&lt; 16
    /// (bit N = band N: 0 = 10 m ... 9 = 160 m, 10 = 630 m, 11 = 2200 m). Agreed with Ron 2026-11-01.
    /// </summary>
    public const byte CMD_GET_CAL_STATUS = 0x2D;
    /// <summary>SWR meter RESET to ms-sdr (data ignored; Pi ms-sdr 3.189+, Windows 3.196+). ms-sdr
    /// sends the HTTP reset to the WiFi meter itself, so RESET works when the client can't reach
    /// the meter (remote / VPN / other subnet). Used when the meter state came from ms-sdr (0x0F).</summary>
    public const byte CMD_SWR_METER_RESET = 0x2E;
    /// <summary>Hard calibration reset to ms-sdr (3.197+, Ron 2026-11-10), 1 byte = kind below.
    /// ms-sdr sets the host files AND the radio, so the radio's old values can't win at the next start.</summary>
    public const byte CMD_SET_CAL_RESET_ALL = 0x2F;
    public const byte CAL_RESET_RX_IQ = 1;   // 0 / 0 (no correction) on all bands
    public const byte CAL_RESET_POWER = 2;   // factory QRP power table
    public const byte CAL_RESET_AMP = 3;     // amplifier cal -99 (not calibrated), host only
    /// <summary>2 TONE on/off (byte 1/0) to ms-sdr, passed to trans (700 + 1900 Hz in the voice path, trans 3.155+).</summary>
    public const byte CMD_SET_TWO_TONE = 0x88;
    /// <summary>CMD_SET_BAND_POWER_POWER (0xA2) data bit: save this QRP value (radio + power_cal.ini).</summary>
    public const int POWER_CAL_SAVE_FLAG = 0x8000;
    public const double IQ_PHASE_STEP_DEG = 0.05;
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
    /// <summary>Extended sub: forward power (u32 milli-watts packing at payload offset 1).</summary>
    public const byte EXT_CMD_SET_FORWARD_POWER = 0x0B;
    /// <summary>Extended sub: reverse power (u32 milli-watts packing at payload offset 1).</summary>
    public const byte EXT_CMD_SET_REVERSE_POWER = 0x0C;
    /// <summary>Extended sub: SWR ×10 as one byte at payload offset 1.</summary>
    public const byte EXT_CMD_SET_SWR = 0x0D;
    /// <summary>Extended sub (Pi ms-sdr 3.188): WiFi SWR meter state relayed by ms-sdr.
    /// Payload after the sub: [fault 0/1][online 0/1][IP a][IP b][IP c][IP d] (0.0.0.0 = unknown).</summary>
    public const byte EXT_CMD_SET_SWR_FAULT = 0x0F;
    /// <summary>Extended sub: server message text from ms-sdr ([0x0B][0x30][ASCII text]).</summary>
    public const byte EXT_CMD_SET_SERVER_ERROR = 0x30;

    // TODO: add more as reverse-engineered (status, extended power, temps, drift, antenna, etc.)
}
