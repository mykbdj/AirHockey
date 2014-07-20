/****************************************************************************//**\file
 * Video Filter Library
 * This library contains function to manipulate color video frames
 *
 * Real-time Embedded Systems ECEN5623, Fall 2013
 * Group7:  Bhardwaj, Bhatt, Joshi, Lopez, Vyas
 ****************************************************************************/
#include "pong.h"

bool filterRedFrame(unsigned char *fullImg, unsigned char *filteredImg, int filterSize) //Red is puck
{

    int i;

    //Code for filtering
    for(i=VIDEO_DIVIDE; i<filterSize;++i) //Skip G and B
    {
        int pixel = i*3;

        if(fullImg[pixel]>(NOISE_MARGIN+fullImg[(pixel)+1]) && (fullImg[pixel]>(NOISE_MARGIN+fullImg[(pixel)+2])))
        {
            filteredImg[i]=255;
        }
        else
        {
            filteredImg[i]=0;
        }
    }

    return true;
}

//This thread is for filtering the paddle which has mapping in just
bool filterGreenFrame(unsigned char *fullImg, unsigned char *filteredImg) //Green is paddle
{

    int i;

    //Code for filtering
    for(i=0; i<VIDEO_DIVIDE;i++)
    {
        int pixel = i*3; //Skip G and B

        if(fullImg[pixel]>(NOISE_MARGIN+fullImg[(pixel)+1]) && (fullImg[i]>(NOISE_MARGIN+fullImg[(pixel)+2])))
        {
            filteredImg[i]=255;
        }
        else
        {
            filteredImg[i]=0;
        }
    }

    return true;
}

bool puckCentroid(unsigned char *img, int wd, int ht, Coords_t *coords, int pongId)
{
    unsigned int inTarget=0, xCnt=0, xCntPrev=0, xCntPrevPrev=0, xCntSum=0, xEdgeL=0, xEdgeR=0, xCent=0, xCntMax=0;
    unsigned int xCentFinal=400, yCentFinal = 400;        //final centroid position
    unsigned char Filt;

    int i, j, multiplier;

    for(i=PUCK_SCAN_HT_START; i<ht; ++i) //height
    {
        inTarget=0;
        xCnt=0;
        multiplier=i*wd;    //Multiply just once instead of doing multiplication every iteration

        for(j=0; j<(wd-3); ++j) //the start value depends on side A/B //j is width - 7
        {
            Filt = (img[multiplier+j] + img[multiplier+j+1] + img[multiplier+j+2] +
                    img[multiplier+j+3] /*+ img[multiplier+j+4] + img[multiplier+j+5] + img[multiplier+j+6] + img[multiplier+j+7]*/) /4;

            // entered target
            if(!inTarget && (Filt > 128) )
            {
                inTarget=1;
                xCnt=0;
                xEdgeL=(unsigned int)j;
            }
            else if(inTarget && (Filt < 128) )
            {
                inTarget=0;
                xEdgeR=(unsigned int)j;
                xCent = xEdgeL + ((xEdgeR-xEdgeL)/2);
                xCntSum = xCntPrevPrev+xCntPrev+xCnt;

                if(xCntSum > xCntMax)
                {
                    xCntMax = xCntSum;
                    xCentFinal = xCent;
                    yCentFinal = i;
                }

                xCntPrevPrev=xCntPrev;
                xCntPrev=xCnt;
                xCnt=0;
            }
            else if(inTarget)
            {
                xCnt++;
            }
        }
    }



    //Test mapping values into 20x20 grid
    coords->x = xCentFinal/16;
    coords->y = yCentFinal/12;


    //copy values to coords struct
    //coords.x = xCentFinal;
    //coords.y = yCentFinal;

    logw("[%s] Puck - Centroid = %d, %d    GridX=%d  GridY=%d\n",  pongId ? "SideB": "SideA", xCentFinal, yCentFinal, coords->x, coords->y);
    //usleep(1000000);

    return true;
}

int paddleCentroid(unsigned char *img, int wd, int ht, int pongId)
{
    unsigned int xCnt=0, xCntPrev=0, xCntPrevPrev=0, xCntSum=0, xEdgeL=0, xEdgeR=0, xCent=0, xCntMax=0;
    unsigned int xCentFinal= 400;        //final paddle position
    unsigned char Filt;
    unsigned int inTarget=0;

    int i, j, multiplier;

    for(i=0; i<PADDLE_SCAN_HT_END; ++i) //height ht
    {
        inTarget=0;
        xCnt=0;
        multiplier=i*wd;                //Multiply just once instead of doing multiplication every iteration

        for(j=0; j<(wd-3); ++j) //up to 80 only for paddle
        {
            Filt = (img[multiplier+j] + img[multiplier+j+1] + img[multiplier+j+2] +
                    img[multiplier+j+3] /*+ img[multiplier+j+4] + img[multiplier+j+5] + img[multiplier+j+6] + img[multiplier+j+7]*/) /4;

            // entered target
            if(!inTarget && (Filt > 128) )
            {
                inTarget=1;
                xCnt=0;
                xEdgeL=(unsigned int)j;
            }
            else if(inTarget && (Filt < 128) )
            {
                inTarget=0;
                xEdgeR=(unsigned int)j;
                xCent = xEdgeL + ((xEdgeR-xEdgeL)/2);
                xCntSum = xCntPrevPrev+xCntPrev+xCnt;

                if(xCntSum > xCntMax)
                {
                    xCntMax = xCntSum;
                    xCentFinal = xCent;
                }

                xCntPrevPrev=xCntPrev;
                xCntPrev=xCnt;
                xCnt=0;
            }
            else if(inTarget)
            {
                xCnt++;
            }
        }
    }

    xCentFinal /= 16;
    log("[%s] GridX Position of the paddle: %d\n",  pongId ? "SideB": "SideA", xCentFinal);

    //usleep(1000000);

    return xCentFinal;
}
