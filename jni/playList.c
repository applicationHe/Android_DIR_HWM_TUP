#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <android/log.h>
#include "playList.h"
#include "clientNetLib.h"

#define  LOG_TAG "playList.c"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

Hi3510_frame_t *VframeList;
pthread_cond_t cond_var;
pthread_mutex_t cond_var_mutex;

Hi3510_frame_t *AframeList;
pthread_mutex_t Audio_mutex;

int st_initFrameList()
{
	LOGI("%s", __FUNCTION__);
	VframeList = NULL;
	AframeList = NULL;
	pthread_mutex_init(&cond_var_mutex, NULL); //modify by qiaob
	pthread_cond_init(&cond_var, NULL);
	pthread_mutex_init(&Audio_mutex, NULL); //add LinZh107
	return 0;
}

static void freelist(Hi3510_frame_t *List) //add LinZh107
{
	Hi3510_frame_t *tmp = List;
	Hi3510_frame_t *tmp1 = tmp;
	while (tmp1)
	{
		tmp = tmp1;
		if (tmp->data)
		{
			free(tmp->data);
			tmp->data = NULL;
		}
		tmp1 = tmp->next;
		if (tmp != NULL)
		{
			free(tmp);
			tmp = NULL;
		}
	}
}

int st_destroyFrameList()
{
	LOGI("%s", __FUNCTION__);
	pthread_mutex_destroy(&cond_var_mutex);
	pthread_cond_destroy(&cond_var);
	pthread_mutex_destroy(&Audio_mutex); //add LinZh107
	freelist(VframeList);
	freelist(AframeList);
	return 0;
}

//get frame list head
Hi3510_frame_t *st_getFrameListHead(int flags)
{
	//LOGI("%s",__FUNCTION__);

#if 0
	pthread_mutex_lock(&cond_var_mutex);
	if(VframeList == NULL)
	{
		//__android_log_print(ANDROID_LOG_DEBUG, "playList.c", "cuo wu --1--:%s",__FUNCTION__);
		goto error;
	}
	if(VframeList->data == NULL)
	{
		//__android_log_print(ANDROID_LOG_DEBUG, "playList.c", "cuo wu --2--:%s",__FUNCTION__);
		goto error;
	}
	pthread_mutex_unlock(&cond_var_mutex);
	return VframeList;
	error:
	pthread_mutex_unlock(&cond_var_mutex);
	return NULL;
#endif

	static unsigned long prev_Vframe_no = 0;
	static unsigned long prev_Aframe_no = 0;
	switch (flags)
	{
	case VIDEO_FRAME:
		pthread_mutex_lock(&cond_var_mutex);
		if (VframeList == NULL || VframeList->data == NULL)
		{
			LOGE("st_getFrameListHead VframeList == NULL");
			pthread_mutex_unlock(&cond_var_mutex);
			return NULL ;
		}
		pthread_mutex_unlock(&cond_var_mutex);
		if (VframeList->frame_no > prev_Vframe_no + 6)
		{
//			LOGE("死循环");
//			while (NULL != VframeList && (VIDEO_FRAME != VframeList->frame_type)) //frame_type: (1)i_frame, (2)P_frame,(3)audio_frame
//			{
////				LOGE("死循环");
//				LOGI("错误");
//				st_removeFrameListHead(VIDEO_FRAME);
//			}
		}

		if (VframeList != NULL)
			prev_Vframe_no = VframeList->frame_no;
		return VframeList;

	case AUDIO_FRAME:
		//LOGI("st_get_AFrameListHead");
		pthread_mutex_lock(&Audio_mutex);
		if (AframeList == NULL || AframeList->data == NULL)
		{
			//LOGE("AframeList->data == NULL");
			pthread_mutex_unlock(&Audio_mutex);
			return NULL ;
		}
		pthread_mutex_unlock(&Audio_mutex);
		return AframeList;
	}
}

void st_removeFrameListHead(int flags) //LinZh107 曾加语音链表  0:Video  1:Audio
{
//	LOGI("%s",__FUNCTION__);
	Hi3510_frame_t *tmp = NULL;
	switch (flags)
	{
	case VIDEO_FRAME:
		tmp = VframeList;
		pthread_mutex_lock(&cond_var_mutex);
		VframeList = VframeList->next;
		if (tmp)
		{
			if (tmp->data)
			{
				free(tmp->data);
				tmp->data = NULL;
			}
			free(tmp);
			tmp = NULL;
		}
		pthread_mutex_unlock(&cond_var_mutex);
		startRecv();
		break;
	case AUDIO_FRAME:
		tmp = AframeList;
		pthread_mutex_lock(&Audio_mutex);
		if (AframeList->next != NULL)
		{
			AframeList = AframeList->next;
			if (tmp)
			{
				if (tmp->data)
				{
					free(tmp->data);
					tmp->data = NULL;
				}
				free(tmp);
				tmp = NULL;
			}
		}
		pthread_mutex_unlock(&Audio_mutex);
		break;
	}
	return;
}

//input one frame to list tail
int st_inputFrameListTail(char *inputFrame, int size)
{
//	LOGI("%s",__FUNCTION__);
	if (NULL == inputFrame || size <= 0) // 修复3518C部分情况下时间跳跃问题 Date: 2013-12-26 Author: yms
	{
		LOGI("没有数据");
		return 0;
	}
//	LOGI("获取流中.....");
	int i = 1;
	Hi3510_frame_t *new = NULL, *tmp = NULL;
	frame_t *tmp_frame = (frame_t *) inputFrame;
	if (tmp_frame->frame_head.frame_type == AUDIO_FRAME) //音频帧
	{
		pthread_mutex_lock(&Audio_mutex);
		if (AframeList == NULL) //新建链表
		{
			//LOGE("AframeList == NULL");
			LOGI("创建链表");
			new = (Hi3510_frame_t *) malloc(sizeof(Hi3510_frame_t));
			if (new == NULL)
			{
				goto error;
			}
			memcpy(new, &tmp_frame->frame_head, sizeof(frame_head_t));
			new->data = (unsigned char *) malloc((sizeof(char)) * tmp_frame->frame_head.frame_size); //size);
			//LOGI("AudioFrameDataSize = %d", tmp_frame->frame_head.frame_size);
			if (new->data == NULL)
			{
				if (new != NULL)
				{
					free(new);
					new = NULL;
				}
				goto error;
			}
			memset(new->data,0,(sizeof(char)) * tmp_frame->frame_head.frame_size);
			memcpy(new->data, tmp_frame->frame_data, tmp_frame->frame_head.frame_size); //size);
			new->next = NULL;
			AframeList = new;
		}
		else //音频链表已存在
		{
			tmp = AframeList;
			while (tmp->next)
			{
				i++;
				tmp = tmp->next;
			}
			LOGI("i__:%d",i);
			if (i == MAX_FRAME_LIST_NUM)
			{
				goto error;
			}
			new = (Hi3510_frame_t *) malloc(sizeof(Hi3510_frame_t));
			if (new == NULL)
			{
				goto error;
			}
			memcpy(new, &tmp_frame->frame_head, sizeof(frame_head_t));
			new->data = (unsigned char *) malloc(tmp_frame->frame_head.frame_size); //size);
//			LOGI("st_inputFrameListTail callback_recv_size = %d",size); //172
//			LOGI("st_inputFrameListTail frame_head.frame_size = %d",tmp_frame->frame_head.frame_size); //172
			if (new->data == NULL)
			{
				if (new != NULL)
				{
					free(new);
					new = NULL;
				}
				goto error;
			}
			memcpy(new->data, tmp_frame->frame_data, tmp_frame->frame_head.frame_size);
			//LOGI("new audio frame.");
			tmp->next = new;
			new->next = NULL;
		}
	}
	else //视频帧
	{
//		LOGI("视频帧.....");
		pthread_mutex_lock(&cond_var_mutex);
//		LOGI("视频帧1.1.....");
		if (VframeList == NULL) //新建链表
		{
			//LOGE("VframeList == NULL");
			new = (Hi3510_frame_t *) malloc(sizeof(Hi3510_frame_t));
			if (new == NULL)
			{
				LOGI("视频帧2.....");
				goto error;
			}
			memcpy(new, &tmp_frame->frame_head, sizeof(frame_head_t));
			new->data = (unsigned char *) malloc((sizeof(char)) * tmp_frame->frame_head.frame_size); //size);
			if (new->data == NULL)
			{
				if (new != NULL)
				{
					LOGI("视频帧3.....");
					free(new);
					new = NULL;
				}
				goto error;
			}
			memcpy(new->data, tmp_frame->frame_data, tmp_frame->frame_head.frame_size); //size);
			new->next = NULL;
			VframeList = new;
			LOGI("创建视频帧");
		}
		else //视频链表已存在
		{
//			LOGI("视频帧4.....");
			tmp = VframeList;
			while (tmp->next)
			{
				i++;
				tmp = tmp->next;
			}
			if(i==99)
			{
				stopRecv();
//				LOGI("i__:%d",i);
			}
			if (i == MAX_FRAME_LIST_NUM)
			{
//				LOGI("视频帧5.....");
				goto error;
			}
			new = (Hi3510_frame_t *) malloc(sizeof(Hi3510_frame_t));
			if (new == NULL)
			{
				LOGI("视频帧6.....");
				goto error;
			}
			memcpy(new, &tmp_frame->frame_head, sizeof(frame_head_t));
			new->data = (unsigned char *) malloc(tmp_frame->frame_head.frame_size); //size);
			if (new->data == NULL)
			{
				if (new != NULL)
				{
					LOGI("视频帧7.....");
					free(new);
					new = NULL;
				}
				goto error;
			}
			memcpy(new->data, tmp_frame->frame_data, tmp_frame->frame_head.frame_size);
			//LOGI("new video frame.");
			tmp->next = new;
			new->next = NULL;
		} //end 视频帧
	} //end 音视频帧
	error:
	{
		if (tmp_frame->frame_head.frame_type == AUDIO_FRAME) //音频帧
			pthread_mutex_unlock(&Audio_mutex);
		else
			pthread_mutex_unlock(&cond_var_mutex);
	}

	return 0;
}

#if 0
int st_inputFrameListTail(char *inputFrame,int size)
{
	if (!m_bVideoPlay)
	return E_FAIL;

	frame_head_t *pFrameHead = (frame_head_t *)pFrame;
	//ATLTRACE("------------m_nFrameNoOld=%d\n", m_nFrameNoOld);
	//add by fengzx 2012-02-13 ,帧号不连续就不送
	if(m_nFrameNoOld == 0)//第一次
	{
		//ATLTRACE("----------11111111111111111%d\n");
		m_nFrameNoOld = pFrameHead->frame_no;
	}
	else
	{
		if(m_nFrameNoOld != pFrameHead->frame_no - 1)  //帧不连续，不是I帧就不送
		{
			m_nFrameNoOld = pFrameHead->frame_no;
			if(pFrameHead->frame_type != I_FRAME_TYPE)
			{
				//		return E_FAIL;
			}
		}
		m_nFrameNoOld = pFrameHead->frame_no;
		//ATLTRACE("---------33333333333333333333333-%d\n", pFrameHead->frame_no);
	}
	//end ///////////////////////////////////
	if (pFrameHead->frame_type != A_FRAME_TYPE)
	{
		//ATLTRACE("uFrameQueueSize = %d, time = %u, m_bRealTime = %d\n",m_FrameQueue.size(), timeGetTime(), m_bRealTime);
	}
	if (pFrameHead == NULL || pFrameHead->frame_size > MAX_FRAME_SIZE)
	return E_FAIL;

	CAutoLock cslock(m_FrameQueue_cs);
	//ATLTRACE("pFrameHead->frame_type = %d\n", pFrameHead->frame_type);
	switch (pFrameHead->frame_type)
	{
		case I_FRAME_TYPE:
		case P_FRAME_TYPE:
		break;
		case A_FRAME_TYPE:
		if (m_bAudioPlay || m_bRecord)
		break;
		default:
		return S_OK;
	}

	if (m_bRecord)
	Record((FRAME_T *)pFrameHead);

	size_t uFrameQueueSize = m_FrameQueue.size();
	if (uFrameQueueSize >= m_uMaxFrameQueueSize)
	{
		ATLTRACE("MaxVideoFrameQueueSize\n");
		do
		{
			FRAME_T *pQueueFrame = m_FrameQueue.front();
			if (pQueueFrame->frame_head.frame_type == I_FRAME_TYPE && uFrameQueueSize < m_uMaxFrameQueueSize*3/4)
			break;
			m_FrameQueue.pop_front();
			delete pQueueFrame;
			pQueueFrame = NULL;
		}while (--uFrameQueueSize > 0);
	}

	DWORD dwQueueFrameSize = sizeof(frame_head_t) + pFrameHead->frame_size;

	std::auto_ptr<char> pQueueFrame(new char[dwQueueFrameSize]);
	if (pQueueFrame.get() == NULL)
	return E_FAIL;

	memcpy_s(pQueueFrame.get(), dwQueueFrameSize, pFrame, dwQueueFrameSize);

	m_FrameQueue.push_back((FRAME_T *)pQueueFrame.release());

	return S_OK;
}
#endif

void st_setFrameListBroadcast()
{
	pthread_mutex_lock(&cond_var_mutex);
	pthread_cond_broadcast(&cond_var);
	pthread_mutex_unlock(&cond_var_mutex);
}

int getFrameNum(int flags) //2014-5-28 modify LinZh107
{
	__android_log_print(ANDROID_LOG_DEBUG, "playList.c", "--:%s", __FUNCTION__);
	int num = 0;
	Hi3510_frame_t *tmp = NULL;
	switch (flags)
	{
	case 1:
		tmp = VframeList;
		break;
	case 3:
		tmp = AframeList;
		break;
	}

	while (tmp)
	{
		tmp = tmp->next;
		num++;
	}
	return num;
}
