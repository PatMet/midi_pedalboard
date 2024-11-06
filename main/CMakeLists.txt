idf_component_register(SRCS "rgb_led.cpp" "usb.cpp" "usb_midi.cpp" "midi_pedalboard.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES mcp23017_driver i2c_cxx_itf usb)
