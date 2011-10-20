/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*Copyright 2009-2011 Freescale Semiconductor, Inc. All Rights Reserved.*/


#include <hardware/hardware.h>

#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#include <hardware/hwcomposer.h>

#include <EGL/egl.h>
#include "gralloc_priv.h"
#include "hwc_common.h"
#include "blit_ipu.h"
extern "C" {
#include "mxc_ipu_hl_lib.h"
}
/*****************************************************************************/
using namespace android;

int blit_device::isIPUDevice(const char *dev_name)
{
		return !strcmp(dev_name, BLIT_IPU);
}

int blit_device::isGPUDevice(const char *dev_name)
{
		return !strcmp(dev_name, BLIT_GPU);
}

blit_ipu::blit_ipu()
{
		init();
}

blit_ipu::~blit_ipu()
{
		uninit();
}

int blit_ipu::init()//, hwc_layer_t *layer, struct output_device *output
{
		//int status = -EINVAL;

    return 0;
}

int blit_ipu::uninit()
{
	  //int status = -EINVAL;

	  return 0;
}

static void fill_buffer(char *pbuf, int len)
{
    static int k = 0;
    short * pframe = (short *)pbuf;
    if(k == 0) {
        for(int i=0; i<len; i+=2) {
            *pframe = 0xf800;
        }
    }

    if(k == 1){
        for(int i=0; i<len; i+=2) {
            *pframe = 0x001f;
        }
    }

    if(k == 2){
        for(int i=0; i<len; i+=2) {
            *pframe = 0x07E0;
        }
    }

    k = (k+1)%3;
}

int blit_ipu::blit(hwc_layer_t *layer, hwc_buffer *out_buf)
{
	  int status = -EINVAL;
	  if(layer == NULL || out_buf == NULL){
	  	  HWCOMPOSER_LOG_ERR("Error!invalid parameters!");
	  	  return status;
	  }
	  //struct blit_ipu *ipu = (struct blit_ipu *)dev;

HWCOMPOSER_LOG_RUNTIME("^^^^^^^^^^^^^^^blit_ipu::blit()^^^^^^^^^^^^^^^^^^^^^^");
	  hwc_rect_t *src_crop = &(layer->sourceCrop);
	  hwc_rect_t *disp_frame = &(layer->displayFrame);
	  private_handle_t *handle = (private_handle_t *)(layer->handle);

    //fill_buffer((char *)(handle->base), handle->size);

	  mIPUInputParam.width = handle->width;//src_crop->right - src_crop->left;
	  mIPUInputParam.height = handle->height;//src_crop->bottom - src_crop->top;
	  mIPUInputParam.input_crop_win.pos.x = src_crop->left;
    mIPUInputParam.input_crop_win.pos.y = src_crop->top;
    mIPUInputParam.input_crop_win.win_w = src_crop->right - src_crop->left;
    mIPUInputParam.input_crop_win.win_h = src_crop->bottom - src_crop->top;

    if(handle->format == HAL_PIXEL_FORMAT_YCbCr_420_SP) {
HWCOMPOSER_LOG_RUNTIME("^^^^^^^^handle->format= NV12");
        mIPUInputParam.fmt = v4l2_fourcc('N', 'V', '1', '2');
    }
    else if(handle->format == HAL_PIXEL_FORMAT_YCbCr_420_I) {
HWCOMPOSER_LOG_RUNTIME("^^^^^^^^handle->format= I420");
        mIPUInputParam.fmt = v4l2_fourcc('I', '4', '2', '0');
    }
    else if((handle->format == HAL_PIXEL_FORMAT_RGB_565) || (handle->format == BLIT_PIXEL_FORMAT_RGB_565)) {
HWCOMPOSER_LOG_RUNTIME("^^^^^^^^handle->format= RGBP");
        mIPUInputParam.fmt = v4l2_fourcc('R', 'G', 'B', 'P');
        //mIPUInputParam.fmt = v4l2_fourcc('N', 'V', '1', '2');
    }else{
        HWCOMPOSER_LOG_ERR("Error!Not supported input format %d",handle->format);
        return status;
    }
#if 0
    if(handle->base != 0) {
       int *pVal = (int *)handle->base;
       HWCOMPOSER_LOG_RUNTIME("=========buff[%d]=%x, buff[%d]=%x, phy=%x", 0, pVal[0], 1, pVal[1], handle->phys);
    }
#endif
    mIPUInputParam.user_def_paddr[0] = handle->phys;
    //out_buf should has width and height to be checked with the display_frame.
//HWCOMPOSER_LOG_ERR("^^^^^^^^in^^paddr=%x^^^^^^left=%d, top=%d, right=%d, bottom=%d", handle->phys, src_crop->left, src_crop->top, src_crop->right, src_crop->bottom);
    mIPUOutputParam.fmt = out_buf->format;//v4l2_fourcc('U', 'Y', 'V', 'Y');
    mIPUOutputParam.show_to_fb = 0;
//HWCOMPOSER_LOG_RUNTIME("^^^^^^^^out_buf->format= %x, out->phy_addr=%x, in->phys=%x", out_buf->format, out_buf->phy_addr, handle->phys);
    if(out_buf->usage & GRALLOC_USAGE_DISPLAY_MASK) {
	    mIPUOutputParam.width = out_buf->width;
	    mIPUOutputParam.height = out_buf->height;
		mIPUOutputParam.output_win.pos.x = 0;
		mIPUOutputParam.output_win.pos.y = 0;
		mIPUOutputParam.output_win.win_w = out_buf->width;
		mIPUOutputParam.output_win.win_h = out_buf->height;
    }
    else if((out_buf->usage & GRALLOC_USAGE_HWC_OVERLAY_DISP1) && (out_buf->width != m_def_disp_w || out_buf->height!= m_def_disp_h)){
            int def_w,def_h;
            int dst_w = out_buf->width;
            int dst_h = out_buf->height;

            mIPUOutputParam.width = out_buf->width;//disp_frame->right - disp_frame->left;
            mIPUOutputParam.height = out_buf->height;//disp_frame->bottom - disp_frame->top;

            if(layer->transform == 0 || layer->transform == 3)
            {
                 def_w = m_def_disp_w;
                 def_h = m_def_disp_h;

                 mIPUOutputParam.output_win.pos.x = (disp_frame->left >> 3) << 3;
                 mIPUOutputParam.output_win.pos.y = (disp_frame->top >> 3) << 3;
                 mIPUOutputParam.output_win.win_w = ((disp_frame->right - disp_frame->left) >> 3) << 3;
                 mIPUOutputParam.output_win.win_h = ((disp_frame->bottom - disp_frame->top) >> 3) << 3;
            }
            else
            {
                 def_w = m_def_disp_h;
                 def_h = m_def_disp_w;

                 mIPUOutputParam.output_win.pos.y = (disp_frame->left >> 3) << 3;
                 mIPUOutputParam.output_win.pos.x = (disp_frame->top >> 3) << 3;
                 mIPUOutputParam.output_win.win_h = ((disp_frame->right - disp_frame->left) >> 3) << 3;
                 mIPUOutputParam.output_win.win_w = ((disp_frame->bottom - disp_frame->top) >> 3) << 3;
             }
             if(dst_w >= dst_h*def_w/def_h){
                 dst_w = dst_h*def_w/def_h;
             }
             else{
                 dst_h = dst_w*def_h/def_w;
             }

            mIPUOutputParam.output_win.pos.x = mIPUOutputParam.output_win.pos.x * dst_w / def_w;
            mIPUOutputParam.output_win.pos.y = mIPUOutputParam.output_win.pos.y * dst_h / def_h;
            mIPUOutputParam.output_win.win_w = mIPUOutputParam.output_win.win_w * dst_w / def_w;
            mIPUOutputParam.output_win.win_h = mIPUOutputParam.output_win.win_h * dst_h / def_h;
            mIPUOutputParam.output_win.pos.x += (out_buf->width - dst_w) >> 1;
            mIPUOutputParam.output_win.pos.y += (out_buf->height - dst_h) >> 1;

            mIPUOutputParam.output_win.pos.x = (mIPUOutputParam.output_win.pos.x >> 3) << 3;
            mIPUOutputParam.output_win.pos.y = (mIPUOutputParam.output_win.pos.y >> 3) << 3;
            mIPUOutputParam.output_win.win_w = (mIPUOutputParam.output_win.win_w >> 3) << 3;
            mIPUOutputParam.output_win.win_h = (mIPUOutputParam.output_win.win_h >> 3) << 3;
    }
    else {
	    mIPUOutputParam.width = out_buf->width;//disp_frame->right - disp_frame->left;
	    mIPUOutputParam.height = out_buf->height;//disp_frame->bottom - disp_frame->top;
	    mIPUOutputParam.output_win.pos.x = (disp_frame->left >> 3) << 3;
	    mIPUOutputParam.output_win.pos.y = (disp_frame->top >> 3) << 3;
	    mIPUOutputParam.output_win.win_w = ((disp_frame->right - disp_frame->left) >> 3) << 3;
	    mIPUOutputParam.output_win.win_h = ((disp_frame->bottom - disp_frame->top) >> 3) << 3;
  	}

//HWCOMPOSER_LOG_ERR("^^^^^^^^out^^paddr=%x^^^^^^left=%d, top=%d, right=%d, bottom=%d", out_buf->phy_addr, disp_frame->left, disp_frame->top, disp_frame->right, disp_frame->bottom);
    mIPUOutputParam.rot = layer->transform;
    if(out_buf->usage & GRALLOC_USAGE_HWC_OVERLAY_DISP1) mIPUOutputParam.rot = 0;

    mIPUOutputParam.user_def_paddr[0] = out_buf->phy_addr;
HWCOMPOSER_LOG_RUNTIME("------mxc_ipu_lib_task_init-----in blit_ipu::blit()------\n");
    if(out_buf->usage & GRALLOC_USAGE_DISPLAY_MASK)
        status = mxc_ipu_lib_task_init(&mIPUInputParam,NULL,&mIPUOutputParam,OP_NORMAL_MODE|TASK_PP_MODE,&mIPUHandle);
    else
        status = mxc_ipu_lib_task_init(&mIPUInputParam,NULL,&mIPUOutputParam,OP_NORMAL_MODE|TASK_ENC_MODE,&mIPUHandle);
	  if(status < 0) {
	  		HWCOMPOSER_LOG_ERR("Error!mxc_ipu_lib_task_init failed %d",status);
	  		return status;
	  }
HWCOMPOSER_LOG_RUNTIME("------mxc_ipu_lib_task_buf_update-----in blit_ipu::blit()------\n");
	  status = mxc_ipu_lib_task_buf_update(&mIPUHandle, handle->phys, out_buf->phy_addr, NULL,NULL,NULL);
	  if(status < 0) {
	  		HWCOMPOSER_LOG_ERR("Error!mxc_ipu_lib_task_buf_update failed %d",status);
	  		return status;
	  }
HWCOMPOSER_LOG_RUNTIME("------mxc_ipu_lib_task_uninit-----in blit_ipu::blit()------\n");
		mxc_ipu_lib_task_uninit(&mIPUHandle);
		status = 0;
        HWCOMPOSER_LOG_RUNTIME("^^^^^^^^^^^^^^^blit_ipu::blit()^^end^^^^^^^^^^^^^^^^^^^^");
	  return status;
}
