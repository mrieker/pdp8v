create_clock -name FIFTYMHZ -period 20.0 -waveform { 0.0 10.0 } [get_ports FIFTYMHZ]
derive_clock_uncertainty
