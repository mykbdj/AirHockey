/****************************************************************************//**\file
 * Video Library
 * This library contains video capture functions.
 *
 * Real-time Embedded Systems ECEN5623, Fall 2013
 * Group7:  Bhardwaj, Bhatt, Joshi, Lopez, Vyas
 ****************************************************************************/
#include "pong.h"

//Filename must be full .ppm extension
//bufferaddress is location of buffer which holds data to be written
//numBytes is the number of bytes to be written
int writeImage(char *fileName, int height, int width,char *bufferAddress, int numBytes)
{
        FILE* fout = fopen(fileName, "w"); //writing to video1 file
        log("Writing %s", fileName);

        fprintf(fout, "P6\n%d %d 255\n",width, height);
        fwrite(bufferAddress, numBytes, 1, fout);
        fclose(fout);
        return true;
}



bool setupVideo(char *device, int wd, int ht, vidReturnValue_t *videoData)
{
    struct v4l2_format fmt;
    unsigned int i, n_buffers;

    log("Setup %s", device);
    //videoData->vid_fd = -1;

    videoData->vid_fd = v4l2_open(device, O_RDWR | O_NONBLOCK, 0);
    if ((videoData->vid_fd) < 0) {
            loge("Cannot open device %s %s", device, strerror(errno) );
            return false;
    }

    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = wd;
    fmt.fmt.pix.height = ht;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    xioctl(videoData->vid_fd, VIDIOC_S_FMT, &fmt);
    if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24) {
            loge("Libv4l didn't accept RGB24 format. Can't proceed.\n");
            return false;
    }

    CLEAR(videoData->vid_req);                 //req needs to be returned back to the structure
    videoData->vid_req.count = 2;
    videoData->vid_req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    videoData->vid_req.memory = V4L2_MEMORY_MMAP;
    xioctl(videoData->vid_fd, VIDIOC_REQBUFS, &(videoData->vid_req));

    videoData->vid_buffer_loc = calloc(videoData->vid_req.count, sizeof(buffers_t));        //returns a pointer to memory block allocated of buffers[2] -0 and 1 each of size 8 bytes

    for (n_buffers = 0; n_buffers < videoData->vid_req.count; ++n_buffers) {
        CLEAR(videoData->vid_buf);

        videoData->vid_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        videoData->vid_buf.memory = V4L2_MEMORY_MMAP;
        videoData->vid_buf.index = n_buffers;

        xioctl(videoData->vid_fd, VIDIOC_QUERYBUF, &(videoData->vid_buf));

        videoData->vid_buffer_loc[n_buffers].length = videoData->vid_buf.length;
        videoData->vid_buffer_loc[n_buffers].start = v4l2_mmap(NULL, videoData->vid_buf.length,
                      PROT_READ | PROT_WRITE, MAP_SHARED,
                      videoData->vid_fd, videoData->vid_buf.m.offset);

        if (MAP_FAILED == videoData->vid_buffer_loc[n_buffers].start) {
                loge("mmap %s", strerror(errno) );
                return false;
        }
    }

    for (i = 0; i < n_buffers; ++i)  //integrate in to previous for loop
    {
        CLEAR(videoData->vid_buf);
        videoData->vid_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        videoData->vid_buf.memory = V4L2_MEMORY_MMAP;
        videoData->vid_buf.index = i;
        xioctl(videoData->vid_fd, VIDIOC_QBUF, &(videoData->vid_buf));
    }

    videoData->vid_n_buffers = n_buffers;
    //printf("Setup Camera FD  = %d",videoData->vid_fd);
    return true;
}
//
//void uint8copy(void *dest, void *src, size_t n){
//    uint64_t * ss = (uint64_t) src;
//    uint64_t * dd = (uint64_t) dest;
//    n = n * sizeof(uint8_t)/sizeof(uint64_t);
//    printf("n=%d\n",n);
//    while(n--)
//        *dd++ = *ss++;
//}//end uint8copy()



bool getFrame(unsigned char *vid, vidReturnValue_t *vidStruct)
{
    enum v4l2_buf_type type;
    fd_set fds;
    struct timeval tv;
    int r;
    //FILE *fout;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    //printf("vidStructFD = %d\n",vidStruct->vid_fd);
    xioctl(vidStruct->vid_fd, VIDIOC_STREAMON, &type);

    do {
        FD_ZERO(&fds);
        FD_SET(vidStruct->vid_fd, &fds);

        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(vidStruct->vid_fd + 1, &fds, NULL, NULL, &tv);
    } while ((r == -1 && (errno = EINTR)));
    if (r == -1) {
            loge("select %s", strerror(errno) );
            return false;
        }

    CLEAR(vidStruct->vid_buf);
    vidStruct->vid_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vidStruct->vid_buf.memory = V4L2_MEMORY_MMAP;
    xioctl(vidStruct->vid_fd, VIDIOC_DQBUF, &(vidStruct->vid_buf));            //dequeue the filled buffer..returns the data back into the v4l2_buffer struct buf

    #ifdef PROFILING_ON
        START_PROFILE();
    #endif
    //writing to &pongInstance.videoFrames
    memcpy(vid,vidStruct->vid_buffer_loc[vidStruct->vid_buf.index].start, vidStruct->vid_buf.bytesused); // can use size instead of buf.bytesused

    #ifdef PROFILING_ON
        STOP_PROFILE();
        GET_PROFILE("memcopy");
    #endif

    //log("Camera FD  = %d",vidStruct->vid_fd);

    xioctl(vidStruct->vid_fd, VIDIOC_QBUF, &(vidStruct->vid_buf));

    return true;

}

int closeVideo(vidReturnValue_t *vidStruct)
{
    enum v4l2_buf_type type;
    unsigned int i;

    if(vidStruct->vid_fd == -1) //no device was opened
        return 0;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    //printf("Calling ioctl close on FD  = %d\n",vidStruct->vid_fd);
    xioctl(vidStruct->vid_fd, VIDIOC_STREAMOFF, &type);
    for (i = 0; i < vidStruct->vid_n_buffers ; ++i)
            v4l2_munmap(vidStruct->vid_buffer_loc[i].start, vidStruct->vid_buffer_loc[i].length);
    v4l2_close(vidStruct->vid_fd);

    vidStruct->vid_fd = -1; //to call device open again

    return 1;
}
