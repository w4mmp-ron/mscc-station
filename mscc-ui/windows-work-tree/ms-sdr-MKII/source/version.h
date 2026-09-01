#define COMPILE_DATE __DATE__
#define COMPILE_TIME __TIME__

// {Model}.{[M]M}{Release Number}  (the bytes are reversed)
// Model 0-OSB, 1-Proficio, 2-Geminus,3-MKII-PTT,4-MKII-ATU,5-Proficio-PTT,6-Proficio-ATU
// VERSION_MINOR is auto-incremented on every build (PreBuildEvent).
// Packed wire format: high byte = minor (0-255), low byte = major.
#define VERSION_MAJOR 3
#define VERSION_MINOR 157
#define VERSION_MS_SDRCORE ((((VERSION_MINOR) << 8) & 0xff00) | ((VERSION_MAJOR) & 0x00ff))
