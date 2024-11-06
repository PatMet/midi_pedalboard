#pragma once

#include "esp_log.h"
#include "freertos/task.h"
#include "usb/usb_host.h"  // USB Host library

class UsbHostMidiClient{

public:

    UsbHostMidiClient();
    ~UsbHostMidiClient();

    void install(void);
    bool connected(void);

    void arm_transfert_in(void);

    void send_note(bool note_on, uint8_t note);
    void send_local_control(bool local_ctrl_on);

    void activate_pass_through(bool pass_on);
    void pass_through(void);

    // called by task function
    void register_(void);
    void task_loop(void);

    // called by event_cb given to usb host lib
    void handle_event(const usb_host_client_event_msg_t *event_msg);
    // called by transfert_cb given to usb host lib
    void handle_midi_in_transfert(usb_transfer_t *transfer);
    void handle_midi_out_transfert(usb_transfer_t *transfer);

private:
    TaskHandle_t task_hdl;
    uint8_t actions;
    uint8_t dev_addr;
    usb_host_client_handle_t client_hdl;
    usb_device_handle_t dev_hdl;

    const usb_intf_desc_t *midi_intf_desc;
    const usb_ep_desc_t *midi_in_ep_desc;
    const usb_ep_desc_t *midi_out_ep_desc;
    
    usb_transfer_t *in_xfer;
    usb_transfer_t *out_xfer;

    bool pass_through_on;

    void submit_midi_transfert_out(void);

    void action_open_dev(void);
    void action_close_dev(void);
    void action_transfert_out(void);
    void action_transfert_in(void);
};
