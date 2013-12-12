#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "util.h"

VdpGetProcAddress * vdp_get_proc_address;
VdpGetErrorString * vdp_get_error_string;
VdpGetApiVersion * vdp_get_api_version;
VdpGetInformationString * vdp_get_information_string;
VdpDeviceDestroy * vdp_device_destroy;
VdpGenerateCSCMatrix * vdp_generate_csc_matrix;
VdpVideoSurfaceQueryCapabilities * vdp_video_surface_query_capabilities;
VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities * vdp_video_surface_query_get_put_bits_y_cb_cr_capabilities;
VdpVideoSurfaceCreate * vdp_video_surface_create;
VdpVideoSurfaceDestroy * vdp_video_surface_destroy;
VdpVideoSurfaceGetParameters * vdp_video_surface_get_parameters;
VdpVideoSurfaceGetBitsYCbCr * vdp_video_surface_get_bits_y_cb_cr;
VdpVideoSurfacePutBitsYCbCr * vdp_video_surface_put_bits_y_cb_cr;
VdpOutputSurfaceQueryCapabilities * vdp_output_surface_query_capabilities;
VdpOutputSurfaceQueryGetPutBitsNativeCapabilities * vdp_output_surface_query_get_put_bits_native_capabilities;
VdpOutputSurfaceQueryPutBitsIndexedCapabilities * vdp_output_surface_query_put_bits_indexed_capabilities;
VdpOutputSurfaceQueryPutBitsYCbCrCapabilities * vdp_output_surface_query_put_bits_y_cb_cr_capabilities;
VdpOutputSurfaceCreate * vdp_output_surface_create;
VdpOutputSurfaceDestroy * vdp_output_surface_destroy;
VdpOutputSurfaceGetParameters * vdp_output_surface_get_parameters;
VdpOutputSurfaceGetBitsNative * vdp_output_surface_get_bits_native;
VdpOutputSurfacePutBitsNative * vdp_output_surface_put_bits_native;
VdpOutputSurfacePutBitsIndexed * vdp_output_surface_put_bits_indexed;
VdpOutputSurfacePutBitsYCbCr * vdp_output_surface_put_bits_y_cb_cr;
VdpBitmapSurfaceQueryCapabilities * vdp_bitmap_surface_query_capabilities;
VdpBitmapSurfaceCreate * vdp_bitmap_surface_create;
VdpBitmapSurfaceDestroy * vdp_bitmap_surface_destroy;
VdpBitmapSurfaceGetParameters * vdp_bitmap_surface_get_parameters;
VdpBitmapSurfacePutBitsNative * vdp_bitmap_surface_put_bits_native;
VdpOutputSurfaceRenderOutputSurface * vdp_output_surface_render_output_surface;
VdpOutputSurfaceRenderBitmapSurface * vdp_output_surface_render_bitmap_surface;
VdpDecoderQueryCapabilities * vdp_decoder_query_capabilities;
VdpDecoderCreate * vdp_decoder_create;
VdpDecoderDestroy * vdp_decoder_destroy;
VdpDecoderGetParameters * vdp_decoder_get_parameters;
VdpDecoderRender * vdp_decoder_render;
VdpVideoMixerQueryFeatureSupport * vdp_video_mixer_query_feature_support;
VdpVideoMixerQueryParameterSupport * vdp_video_mixer_query_parameter_support;
VdpVideoMixerQueryAttributeSupport * vdp_video_mixer_query_attribute_support;
VdpVideoMixerQueryParameterValueRange * vdp_video_mixer_query_parameter_value_range;
VdpVideoMixerQueryAttributeValueRange * vdp_video_mixer_query_attribute_value_range;
VdpVideoMixerCreate * vdp_video_mixer_create;
VdpVideoMixerSetFeatureEnables * vdp_video_mixer_set_feature_enables;
VdpVideoMixerSetAttributeValues * vdp_video_mixer_set_attribute_values;
VdpVideoMixerGetFeatureSupport * vdp_video_mixer_get_feature_support;
VdpVideoMixerGetFeatureEnables * vdp_video_mixer_get_feature_enables;
VdpVideoMixerGetParameterValues * vdp_video_mixer_get_parameter_values;
VdpVideoMixerGetAttributeValues * vdp_video_mixer_get_attribute_values;
VdpVideoMixerDestroy * vdp_video_mixer_destroy;
VdpVideoMixerRender * vdp_video_mixer_render;
VdpPresentationQueueTargetDestroy * vdp_presentation_queue_target_destroy;
VdpPresentationQueueCreate * vdp_presentation_queue_create;
VdpPresentationQueueDestroy * vdp_presentation_queue_destroy;
VdpPresentationQueueSetBackgroundColor * vdp_presentation_queue_set_background_color;
VdpPresentationQueueGetBackgroundColor * vdp_presentation_queue_get_background_color;
VdpPresentationQueueGetTime * vdp_presentation_queue_get_time;
VdpPresentationQueueDisplay * vdp_presentation_queue_display;
VdpPresentationQueueBlockUntilSurfaceIdle * vdp_presentation_queue_block_until_surface_idle;
VdpPresentationQueueQuerySurfaceStatus * vdp_presentation_queue_query_surface_status;
VdpPreemptionCallbackRegister * vdp_preemption_callback_register;
VdpPresentationQueueTargetCreateX11 * vdp_presentation_queue_target_create_x11;

static void load_vdpau(VdpDevice dev);

static const int input_width = 1440;
static const int input_height = 1152;
static const int output_width = 1440;
static const int output_height = 1152;

VdpDevice dev;

#define ok(a) do { ret = a; if (ret != VDP_STATUS_OK) { fprintf(stderr, "%s:%u %u %s\n", __FUNCTION__, __LINE__, ret, vdp_get_error_string(ret)); exit(1); } } while (0)

int main(int argc, char *argv[]) {
   Display *display;
   Window root, window;
   VdpStatus ret;
   VdpDecoder dec;

   /* This requires libvdpau_trace, which is available in libvdpau.git */
   setenv("VDPAU_TRACE", "1", 0);

   display = XOpenDisplay(NULL);
   root = XDefaultRootWindow(display);
   window = XCreateSimpleWindow(display, root, 0, 0, output_width, output_height, 0, 0, 0);
   XSelectInput(display, window, ExposureMask | KeyPressMask);
   XMapWindow(display, window);
   XSync(display, 0);

   ret = vdp_device_create_x11(display, 0, &dev, &vdp_get_proc_address);
   assert(ret == VDP_STATUS_OK);

   load_vdpau(dev);

   ok(vdp_decoder_create(dev, VDP_DECODER_PROFILE_MPEG2_MAIN, input_width, input_height, 2, &dec));
   vdp_decoder_destroy(dec);

   ok(vdp_decoder_create(dev, VDP_DECODER_PROFILE_VC1_MAIN, input_width, input_height, 2, &dec));
   vdp_decoder_destroy(dec);

   ok(vdp_decoder_create(dev, VDP_DECODER_PROFILE_H264_MAIN, input_width, input_height, 2, &dec));
   vdp_decoder_destroy(dec);

   ok(vdp_decoder_create(dev, VDP_DECODER_PROFILE_MPEG4_PART2_ASP, input_width, input_height, 2, &dec));
   vdp_decoder_destroy(dec);

   vdp_device_destroy(dev);
   XDestroyWindow(display, window);
   XCloseDisplay(display);
   return 0;
}

static void load_vdpau(VdpDevice dev) {
   VdpStatus ret;
#define GET_POINTER(fid, fn) ret = vdp_get_proc_address(dev, fid, (void**)&fn); assert(ret == VDP_STATUS_OK);
   GET_POINTER(VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER,                          vdp_preemption_callback_register);
   GET_POINTER(VDP_FUNC_ID_GET_ERROR_STRING,                                      vdp_get_error_string);
   GET_POINTER(VDP_FUNC_ID_GET_API_VERSION,                                       vdp_get_api_version);
   GET_POINTER(VDP_FUNC_ID_GET_INFORMATION_STRING,                                vdp_get_information_string);
   GET_POINTER(VDP_FUNC_ID_DEVICE_DESTROY,                                        vdp_device_destroy);
   GET_POINTER(VDP_FUNC_ID_GENERATE_CSC_MATRIX,                                   vdp_generate_csc_matrix);
   GET_POINTER(VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES,                      vdp_video_surface_query_capabilities);
   GET_POINTER(VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES, vdp_video_surface_query_get_put_bits_y_cb_cr_capabilities);
   GET_POINTER(VDP_FUNC_ID_VIDEO_SURFACE_CREATE,                                  vdp_video_surface_create);
   GET_POINTER(VDP_FUNC_ID_VIDEO_SURFACE_DESTROY,                                 vdp_video_surface_destroy);
   GET_POINTER(VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS,                          vdp_video_surface_get_parameters);
   GET_POINTER(VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR,                        vdp_video_surface_get_bits_y_cb_cr);
   GET_POINTER(VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR,                        vdp_video_surface_put_bits_y_cb_cr);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES,                     vdp_output_surface_query_capabilities);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES, vdp_output_surface_query_get_put_bits_native_capabilities);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES,    vdp_output_surface_query_put_bits_indexed_capabilities);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES,    vdp_output_surface_query_put_bits_y_cb_cr_capabilities);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_CREATE,                                 vdp_output_surface_create);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY,                                vdp_output_surface_destroy);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_GET_PARAMETERS,                         vdp_output_surface_get_parameters);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE,                        vdp_output_surface_get_bits_native);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE,                        vdp_output_surface_put_bits_native);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED,                       vdp_output_surface_put_bits_indexed);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR,                       vdp_output_surface_put_bits_y_cb_cr);
   GET_POINTER(VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES,                     vdp_bitmap_surface_query_capabilities);
   GET_POINTER(VDP_FUNC_ID_BITMAP_SURFACE_CREATE,                                 vdp_bitmap_surface_create);
   GET_POINTER(VDP_FUNC_ID_BITMAP_SURFACE_DESTROY,                                vdp_bitmap_surface_destroy);
   GET_POINTER(VDP_FUNC_ID_BITMAP_SURFACE_GET_PARAMETERS,                         vdp_bitmap_surface_get_parameters);
   GET_POINTER(VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE,                        vdp_bitmap_surface_put_bits_native);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE,                  vdp_output_surface_render_output_surface);
   GET_POINTER(VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE,                  vdp_output_surface_render_bitmap_surface);
   GET_POINTER(VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES,                            vdp_decoder_query_capabilities);
   GET_POINTER(VDP_FUNC_ID_DECODER_CREATE,                                        vdp_decoder_create);
   GET_POINTER(VDP_FUNC_ID_DECODER_DESTROY,                                       vdp_decoder_destroy);
   GET_POINTER(VDP_FUNC_ID_DECODER_GET_PARAMETERS,                                vdp_decoder_get_parameters);
   GET_POINTER(VDP_FUNC_ID_DECODER_RENDER,                                        vdp_decoder_render);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT,                     vdp_video_mixer_query_feature_support);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT,                   vdp_video_mixer_query_parameter_support);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT,                   vdp_video_mixer_query_attribute_support);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE,               vdp_video_mixer_query_parameter_value_range);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE,               vdp_video_mixer_query_attribute_value_range);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_CREATE,                                    vdp_video_mixer_create);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES,                       vdp_video_mixer_set_feature_enables);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES,                      vdp_video_mixer_set_attribute_values);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_SUPPORT,                       vdp_video_mixer_get_feature_support);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_ENABLES,                       vdp_video_mixer_get_feature_enables);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_GET_PARAMETER_VALUES,                      vdp_video_mixer_get_parameter_values);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_GET_ATTRIBUTE_VALUES,                      vdp_video_mixer_get_attribute_values);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_DESTROY,                                   vdp_video_mixer_destroy);
   GET_POINTER(VDP_FUNC_ID_VIDEO_MIXER_RENDER,                                    vdp_video_mixer_render);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY,                     vdp_presentation_queue_target_destroy);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE,                             vdp_presentation_queue_create);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY,                            vdp_presentation_queue_destroy);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR,               vdp_presentation_queue_set_background_color);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_GET_BACKGROUND_COLOR,               vdp_presentation_queue_get_background_color);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME,                           vdp_presentation_queue_get_time);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY,                            vdp_presentation_queue_display);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE,           vdp_presentation_queue_block_until_surface_idle);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS,               vdp_presentation_queue_query_surface_status);
   GET_POINTER(VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER,                          vdp_preemption_callback_register);
   GET_POINTER(VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_CREATE_X11,                  vdp_presentation_queue_target_create_x11);
}
