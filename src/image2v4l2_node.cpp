#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <linux/videodev2.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/ioctl.h>

#define VIDEO_DEVICE "/dev/video0"
#define FRAME_WIDTH  2560
#define FRAME_HEIGHT 1920

#define FRAME_SIZE (FRAME_WIDTH * FRAME_HEIGHT * 3)

class Image2V4L2
{
  ros::NodeHandle nh_;
  image_transport::ImageTransport it_;
  image_transport::Subscriber image_sub_;

  struct v4l2_capability vid_caps;
  struct v4l2_format vid_format;

  int fdwr;

public:
  Image2V4L2()
    : it_(nh_)
  {
    // Subscrive to input video feed and publish output video feed
    image_sub_ = it_.subscribe("/camera1/image_raw", 1, 
      &Image2V4L2::imageCb, this);

	  fdwr = open(VIDEO_DEVICE, O_RDWR);
    assert(fdwr >= 0);

    int ret_code = ioctl(fdwr, VIDIOC_QUERYCAP, &vid_caps);
    assert(ret_code != -1);



    //cv::namedWindow(OPENCV_WINDOW);
  }

  ~Image2V4L2()
  {
	  close(fdwr);
  }

  void imageCb(const sensor_msgs::ImageConstPtr& msg)
  {
    cv_bridge::CvImagePtr cv_ptr;
    try
    {
      cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
    }
    catch (cv_bridge::Exception& e)
    {
      ROS_ERROR("cv_bridge exception: %s", e.what());
      return;
    }

    memset(&vid_format, 0, sizeof(vid_format));

    vid_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    vid_format.fmt.pix.width = FRAME_WIDTH;
    vid_format.fmt.pix.height = FRAME_HEIGHT;
    vid_format.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
    vid_format.fmt.pix.sizeimage = FRAME_WIDTH * FRAME_HEIGHT * 3;
    vid_format.fmt.pix.field = V4L2_FIELD_NONE;
    vid_format.fmt.pix.bytesperline = FRAME_WIDTH * 3;
    vid_format.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
    int ret_code = ioctl(fdwr, VIDIOC_S_FMT, &vid_format);
    assert(ret_code != -1);

    write(fdwr, cv_ptr->image.data, FRAME_SIZE);

  }
};

int main(int argc, char** argv)
{
  ros::init(argc, argv, "image_converter");
  Image2V4L2 ic;
  ros::spin();
  return 0;
}
