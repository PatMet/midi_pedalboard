
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "usb/usb_host.h"  // USB Host library

#include "usb_midi.hpp"

static const char TAG[] = "pedalboard:usb_midi";

// bit mask for each action
#define MIDI_CLASS_DRIVER_ACTION_OPEN_DEV     0x01
#define MIDI_CLASS_DRIVER_ACTION_CLOSE_DEV    0x02
#define MIDI_CLASS_DRIVER_ACTION_TRANSFER_OUT 0x04
#define MIDI_CLASS_DRIVER_ACTION_TRANSFER_IN  0x08

static void usb_host_midi_client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg);
static void usb_client_midi_out_transfer_cb(usb_transfer_t *transfer);
static void usb_client_midi_in_transfer_cb(usb_transfer_t *transfer);

UsbHostMidiClient::UsbHostMidiClient():
task_hdl{NULL},
actions{0},
dev_addr{0},
client_hdl{NULL},
dev_hdl{NULL},
midi_intf_desc{NULL},
midi_in_ep_desc{NULL},
midi_out_ep_desc{NULL},
in_xfer{NULL},
out_xfer{NULL},
pass_through_on{false}
{
    install();
}

UsbHostMidiClient::~UsbHostMidiClient()
{
}

void usb_host_midi_client_task(void *arg)
{
    UsbHostMidiClient *midi_client_p = static_cast<UsbHostMidiClient*>(arg);
    midi_client_p->register_();
    midi_client_p->task_loop();
}

void UsbHostMidiClient::install(void){

    ESP_LOGD(TAG, "Installing MIDI class driver");
    //ESP_ERROR_CHECK(midi_acd_host_install());

    // Create a task that will handle USB midi client events
    xTaskCreate(usb_host_midi_client_task, "usb_midi", 4096, static_cast<void*>(this), 10, &task_hdl);
}

static void usb_host_midi_client_event_cb(const usb_host_client_event_msg_t *event_msg, void *arg)
{
    //This is function is called from within usb_host_client_handle_events(). Don't block and try to keep it short
    UsbHostMidiClient *midi_client_p = static_cast<UsbHostMidiClient*>(arg);
    midi_client_p->handle_event(event_msg);
}

void UsbHostMidiClient::handle_event(const usb_host_client_event_msg_t *event_msg)
{
    ESP_LOGD(TAG, "Handling event %d", event_msg->event);
    switch (event_msg->event) {
        case USB_HOST_CLIENT_EVENT_NEW_DEV:
            if (dev_addr == 0) {
                dev_addr = event_msg->new_dev.address;
                //Open the device next
                actions |= MIDI_CLASS_DRIVER_ACTION_OPEN_DEV;
            }
            break;
        case USB_HOST_CLIENT_EVENT_DEV_GONE:
            if (dev_hdl != NULL) {
                //Cancel any other actions and close the device next
                actions = MIDI_CLASS_DRIVER_ACTION_CLOSE_DEV;
            }
            break;
        default:
            //Should never occur
            abort();
    }
}

#define CLIENT_NUM_EVENT_MSG        5

void UsbHostMidiClient::register_(void)
{
    ESP_LOGD(TAG, "Registering Client");

    usb_host_client_config_t client_config = {
        .is_synchronous = false,    //Synchronous clients currently not supported. Set this to false
        .max_num_event_msg = CLIENT_NUM_EVENT_MSG,
        .async = {
            .client_event_callback = usb_host_midi_client_event_cb,
            .callback_arg = static_cast<void*>(this),
        },
    };
    ESP_ERROR_CHECK(usb_host_client_register(&client_config, &client_hdl));
}
/*
void UsbHostMidiClient::deregister(void)
{
    //Cleanup class driver
    usb_host_transfer_free(transfer);
    usb_host_client_deregister(client_hdl);
}
*/
void UsbHostMidiClient::task_loop(void)
{
    //This is function is called from within usb_host_client_handle_events(). Don't block and try to keep it short
    ESP_LOGD(TAG, "Entering event handling loop");
    while (1) {
        ESP_LOGD(TAG, "New loop with action %d", actions);
        if (actions == 0) {
            usb_host_client_handle_events(client_hdl, portMAX_DELAY);
            ESP_LOGD(TAG, "usb_host_client_handle_events unblocked with actions %d", actions);
        } else {
            if (actions & MIDI_CLASS_DRIVER_ACTION_OPEN_DEV) {
                action_open_dev();
                actions &= ~MIDI_CLASS_DRIVER_ACTION_OPEN_DEV;
            }
            if (actions & MIDI_CLASS_DRIVER_ACTION_CLOSE_DEV) {
                action_close_dev();
                actions &= ~MIDI_CLASS_DRIVER_ACTION_CLOSE_DEV;
            }
            if (actions & MIDI_CLASS_DRIVER_ACTION_TRANSFER_OUT) {
                action_transfert_out();
                actions &= ~MIDI_CLASS_DRIVER_ACTION_TRANSFER_OUT;
            }
            if (actions & MIDI_CLASS_DRIVER_ACTION_TRANSFER_IN) {
                action_transfert_in();
                actions &= ~MIDI_CLASS_DRIVER_ACTION_TRANSFER_IN;
            }
        }
    }
    ESP_LOGI(TAG, "Exiting event handling loop");
}

void UsbHostMidiClient::action_open_dev(void)
{
    assert(dev_addr != 0);
    ESP_LOGI(TAG, "Opening device at address %d", dev_addr);
    ESP_ERROR_CHECK(usb_host_device_open(client_hdl, dev_addr, &dev_hdl));

    // get device info : usefull ?
    ESP_LOGI(TAG, "Getting device information");
    usb_device_info_t dev_info;
    ESP_ERROR_CHECK(usb_host_device_info(dev_hdl, &dev_info));
    ESP_LOGI(TAG, "\t%s speed", (dev_info.speed == USB_SPEED_LOW) ? "Low" : "Full");
    ESP_LOGI(TAG, "\tbConfigurationValue %d", dev_info.bConfigurationValue);
    if (dev_info.str_desc_manufacturer) {
        ESP_LOGI(TAG, "Manufacturer :");
        usb_print_string_descriptor(dev_info.str_desc_manufacturer);
    }
    if (dev_info.str_desc_product) {
        ESP_LOGI(TAG, "Product :");
        usb_print_string_descriptor(dev_info.str_desc_product);
    }
    // check device descriptor
    ESP_LOGI(TAG, "Getting device descriptor");
    const usb_device_desc_t *dev_desc;
    ESP_ERROR_CHECK(usb_host_get_device_descriptor(dev_hdl, &dev_desc));
    usb_print_device_descriptor(dev_desc);
    // check something ?
    assert(dev_desc->bDeviceClass == USB_CLASS_PER_INTERFACE);

    // check config descriptor
    ESP_LOGI(TAG, "Getting device configuration descriptor");
    const usb_config_desc_t *config_desc;
    const usb_intf_desc_t *intf_desc;
    int desc_offset;
    ESP_ERROR_CHECK(usb_host_get_active_config_descriptor(dev_hdl, &config_desc));
    usb_print_config_descriptor(config_desc, NULL);
    // Go through all config's interfaces and parse
    for (int i = 0; i < config_desc->bNumInterfaces; i++) {
        ESP_LOGD(TAG, "Parsing interface #%d", i);
        intf_desc = usb_parse_interface_descriptor(config_desc, i, 0, &desc_offset);

        // looking for these specific class and subclass types
        // bInterfaceClass 0x1 // AUDIO
        // bInterfaceSubClass 0x3 // MIDISTREAMING
        // bInterfaceProtocol 0x0 // PR_PROTOCOL_UNDEFINED
        if ((intf_desc->bInterfaceClass == 0x1) // USB_CLASS_AUDIO
        and (intf_desc->bInterfaceSubClass == 0x3))
        {
            ESP_LOGI(TAG, "MIDI Streaming interface found (interface #%d)", intf_desc->bInterfaceNumber);
            // if ok, claim interface
            ESP_ERROR_CHECK(usb_host_interface_claim(client_hdl, dev_hdl, intf_desc->bInterfaceNumber, 0));
            midi_intf_desc = intf_desc;

            const int temp_offset = desc_offset; // Save this offset for later
            // Go through all interface's endpoints and parse Interrupt and Bulk endpoints
            for (int e = 0; e < intf_desc->bNumEndpoints; e++) {
                const usb_ep_desc_t *this_ep = usb_parse_endpoint_descriptor_by_index(intf_desc, e, config_desc->wTotalLength, &desc_offset);
                ESP_LOGI(TAG, "Checking endpoint #%d :", e);
                // check endpoint transfert type
                if (USB_EP_DESC_GET_XFERTYPE(this_ep) == USB_TRANSFER_TYPE_BULK) {
                    if (USB_EP_DESC_GET_EP_DIR(this_ep))
                    {
                        ESP_LOGI(TAG, "Endpoint %d  IN", USB_EP_DESC_GET_EP_NUM(this_ep));
                        midi_in_ep_desc = this_ep;

                        // Setup IN data transfer
                        usb_host_transfer_alloc(USB_EP_DESC_GET_MPS(midi_in_ep_desc), 0, &in_xfer);
                        assert(in_xfer);
                        in_xfer->num_bytes = USB_EP_DESC_GET_MPS(midi_in_ep_desc);
                        in_xfer->bEndpointAddress = midi_in_ep_desc->bEndpointAddress;
                        in_xfer->device_handle = dev_hdl;
                        in_xfer->callback = usb_client_midi_in_transfer_cb;
                        in_xfer->context = static_cast<void*>(this);
                    }
                    else
                    {
                        ESP_LOGI(TAG, "Endpoint %d  OUT", USB_EP_DESC_GET_EP_NUM(this_ep));
                        midi_out_ep_desc = this_ep;

                        // Setup OUT data transfer
                        usb_host_transfer_alloc(USB_EP_DESC_GET_MPS(midi_out_ep_desc), 0, &out_xfer);
                        assert(out_xfer);
                        out_xfer->num_bytes = USB_EP_DESC_GET_MPS(midi_out_ep_desc);
                        out_xfer->bEndpointAddress = midi_out_ep_desc->bEndpointAddress;
                        out_xfer->device_handle = dev_hdl;
                        out_xfer->callback = usb_client_midi_out_transfer_cb;
                        out_xfer->context = static_cast<void*>(this);
                    }
                }
                desc_offset = temp_offset;
            }
        }
        else
        {
            // else RAZ
            midi_intf_desc = NULL;
        }
    }

    if (midi_intf_desc == NULL) // no appropriate interface found in this device
    {
        ESP_LOGI(TAG, "No MIDI Streaming interface found on device at address %d", dev_addr);
        action_close_dev();
    }
    else // MIDI interface found
    {
        arm_transfert_in();
    }
}



bool UsbHostMidiClient::connected(void){
    return midi_intf_desc != NULL;
}

void UsbHostMidiClient::action_close_dev(void)
{
    if (midi_intf_desc != NULL)
    {
        ESP_LOGI(TAG, "Releasing interface %d", midi_intf_desc->bInterfaceNumber);
        ESP_ERROR_CHECK(usb_host_interface_release(client_hdl, dev_hdl, midi_intf_desc->bInterfaceNumber));
        midi_intf_desc = NULL;
        midi_in_ep_desc = NULL;
        midi_out_ep_desc = NULL;
        
        usb_host_transfer_free(in_xfer);
        in_xfer = NULL;

        usb_host_transfer_free(out_xfer);
        out_xfer = NULL;
    }
    
    ESP_LOGI(TAG, "Closing device");
    ESP_ERROR_CHECK(usb_host_device_close(client_hdl, dev_hdl));
    dev_hdl = NULL;
    dev_addr = 0;
}



void UsbHostMidiClient::send_note(bool note_on, uint8_t note)
{
    if (connected()){
        ESP_LOGD(TAG, "send_note %d %s", note, note_on ? "ON" : "OFF");
        out_xfer->num_bytes = 8;
        out_xfer->data_buffer[0] = 0x08; // delta time
        out_xfer->data_buffer[1] = note_on ? 0x90 : 0x80; // Status : Note ON / OFF
        out_xfer->data_buffer[2] = note; // note
        out_xfer->data_buffer[3] = 0x40; // Velocity 64/127

        out_xfer->data_buffer[4] = 0x08; // delta time
        out_xfer->data_buffer[5] = note_on ? 0x90 : 0x80; // Status : Note ON / OFF
        out_xfer->data_buffer[6] = note+7; // note
        out_xfer->data_buffer[7] = 0x40; // Velocity 64/127

        submit_midi_transfert_out();
    }
    else
    {
        ESP_LOGW(TAG, "send_note : No MIDI device connected");
    }
}

void UsbHostMidiClient::send_local_control(bool local_ctrl_on)
{
    if (connected()){
        ESP_LOGD(TAG, "local_control %s", local_ctrl_on ? "ON" : "OFF");
        out_xfer->num_bytes = 4;
        out_xfer->data_buffer[0] = 0x08; // delta time
        out_xfer->data_buffer[1] = 0xB0; // Status : Local control
        out_xfer->data_buffer[2] = 0x7A;
        out_xfer->data_buffer[3] = local_ctrl_on ? 0x7F : 0x00; // Local ON / OFF

        submit_midi_transfert_out();
    }
    else
    {
        ESP_LOGW(TAG, "local_control : No MIDI device connected");
    }
}

void UsbHostMidiClient::submit_midi_transfert_out(void)
{
//    ESP_LOGI(TAG, "Submitting OUT transfert");
    //Send an OUT transfer to EP1
    usb_host_transfer_submit(out_xfer);
    // TODO : block until OUT transfert done
    printf("MIDI OUT : ");
    for (int i = 0; i < out_xfer->num_bytes; ++i){
        printf("%02X ", out_xfer->data_buffer[i]);
    }
    printf(" (%d bytes)\n", out_xfer->num_bytes);
}

static void usb_client_midi_out_transfer_cb(usb_transfer_t *transfer)
{
    //This is function is called from within usb_host_client_handle_events(). Don't block and try to keep it short
    UsbHostMidiClient *midi_client_p = static_cast<UsbHostMidiClient*>(transfer->context);
    midi_client_p->handle_midi_out_transfert(transfer);
}

void UsbHostMidiClient::handle_midi_out_transfert(usb_transfer_t *transfer)
{
    //ESP_LOGI(TAG, "Handling midi OUT transfert");
    //This is function is called from within usb_host_client_handle_events(). Don't block and try to keep it short
    //printf("Transfer status %d, actual number of bytes transferred %d\n", transfer->status, transfer->actual_num_bytes);
    // TODO : notify end of OUT transfert. 
    actions |= MIDI_CLASS_DRIVER_ACTION_TRANSFER_OUT;
}

void UsbHostMidiClient::action_transfert_out(void)
{
    ESP_LOGD(TAG, "Action on midi OUT transfert");
//    printf("Transfer status %d, actual number of bytes transferred %d\n", out_xfer->status, out_xfer->actual_num_bytes);
}



void UsbHostMidiClient::arm_transfert_in(void)
{
    ESP_LOGD(TAG, "Arming IN transfert");
    //memset(in_xfer->data_buffer, 0xAA, USB_EP_DESC_GET_MPS(midi_in_ep_desc));
    usb_host_transfer_submit(in_xfer);
}

static void usb_client_midi_in_transfer_cb(usb_transfer_t *transfer)
{
    //This is function is called from within usb_host_client_handle_events(). Don't block and try to keep it short
    UsbHostMidiClient *midi_client_p = static_cast<UsbHostMidiClient*>(transfer->context);
    midi_client_p->handle_midi_in_transfert(transfer);
}

void UsbHostMidiClient::handle_midi_in_transfert(usb_transfer_t *transfer)
{
    ESP_LOGD(TAG, "Handling midi IN transfert");
    //This is function is called from within usb_host_client_handle_events(). Don't block and try to keep it short
    actions |= MIDI_CLASS_DRIVER_ACTION_TRANSFER_IN;


    // get ready for next IN transfert
    // FIXME : à déplacer APRES le traitement des données reçues
//    arm_transfert_in();
}

void UsbHostMidiClient::action_transfert_in(void)
{
    ESP_LOGD(TAG, "Action on midi IN transfert");
    printf("MIDI IN : ");
    for (int i = 0; i < in_xfer->actual_num_bytes; ++i){
        printf("%02X ", in_xfer->data_buffer[i]);
    }
    printf(" (%d bytes) (status %d)\n", in_xfer->actual_num_bytes, in_xfer->status);
    pass_through();
    // get ready for next IN transfert
    // FIXME : à déplacer APRES le traitement des données reçues
    arm_transfert_in();
}


void UsbHostMidiClient::activate_pass_through(bool pass_on){
    ESP_LOGI(TAG, "Pass through midi IN -> OUT %s", pass_on ? "Enabled" : "Disabled");
    pass_through_on = pass_on;
}

void UsbHostMidiClient::pass_through(void){
    if (pass_through_on){
        // Reeived MIDI IN message copy to Midi OUT
        for (int i = 0; i < in_xfer->actual_num_bytes; ++i){
            out_xfer->data_buffer[i] = in_xfer->data_buffer[i];
        }
        out_xfer->num_bytes = in_xfer->actual_num_bytes;
        // send message
        submit_midi_transfert_out();
    }
}
