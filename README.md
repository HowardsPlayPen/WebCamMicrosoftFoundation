# Webcam Capture with the Media Foundation SDK

This code was downloaded from  https://elcharolin.wordpress.com/2017/08/28/webcam-capture-with-the-media-foundation-sdk/ and was a good introduction to accessing Webcams using Microsoft Foundation api calls but it contained a number of nasty issues that caught me out. 

Note: there is a Google Drive [link](https://drive.google.com/file/d/0B4_E-yC5R_ScZlduenFBSGJFUVE/view?resourcekey=0-5S-VdOqfpCwAnsAT14DKXw) at the bottom of the page to download the source. I checked in my updates to GitHub to have easier tracking of my changes


So I thought I would update and annotate the code to show what I found.

## RGB vs YUY2 NV12 etc

Inside Media::IsMediaTypeSupported it checks if the native media type is one of a list of options, including YUY2 and NV12  BUT it then makes a gross assumption when displaying the pixel data that it is RGB. As others have seen this basically means it will exception nastily (both my webcams natively support YUY2 and so the code did not work out of the box)

## I found Media::IsMediaTypeSupported confusing

It returned S_OK for formats that it clearly did not support (as it only supported RGB24).
It returned S_FALSE for everything else - but in Media::SetSourceReader where it called IsMediaSupported it checks for FAILED on this return code (but S_FALSE is still a success in COM speak) 

IT ALSO then checks for SUCCESS(hr) to see if it had found a supported media type - but as above it would think SUCCESS(S_FALSE) had worked (!!). So it would stop looping even though it had not found a supported type!

## I added an implementation of YUY2 to RGBA (TransformImage_YUY2)

I merely referred to an implementation on the Intel website https://software.intel.com/sites/landingpage/mmsf/documentation/preview_8cpp.html#af3d9de67be8e955b824d4b497bba4c96