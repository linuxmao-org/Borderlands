//------------------------------------------------------------------------------
// FRONTIÈRES:  An interactive granular sampler.
//------------------------------------------------------------------------------
// More information is available at
//     http::/ccrma.stanford.edu/~carlsonc/256a/Borderlands/index.html
//
//
// Copyright (C) 2011  Christopher Carlson
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 3.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


//
//  SampleVis.cpp
//  Frontières
//
//  Created by Christopher Carlson on 11/30/11.
//


// TODO:  set and show name implementation

#include "SampleVis.h"
#include "utility/GTime.h"

// TODO avoid this
#include "interface/MyGLApplication.h"
#include "interface/MyGLWindow.h"

#include "model/Sample.h"

// graphics includes
#include <qopengl.h>

// destructor
SampleVis::~SampleVis()
{
    myBuff = NULL;
}


// constructor
SampleVis::SampleVis()
{
    // initializtion
    init();

    // get screen width and height
    MyGLScreen *screen = theApplication->GLwindow()->screen();
    unsigned screenWidth = screen->width();
    unsigned screenHeight = screen->height();

    float xBorder = 100.0;
    float yBorder = 50.0;
    // translation coordinates
    rX = xBorder + ((float)rand() / RAND_MAX) * (screenWidth - xBorder * 2.0);
    rY = yBorder + ((float)rand() / RAND_MAX) * (screenHeight - yBorder * 2.0);

    // min w and h dim
    minDim = 60.0f;

    // scale factor for randomization
    float scaleF = 0.5;

    // box corners
    setWidthHeight(minDim + scaleF * ((float)rand() / RAND_MAX) * screenWidth,
                   minDim + scaleF * ((float)rand() / RAND_MAX) * screenHeight);

    // set orientation
    if (randf() < 0.5)
        orientation = true;  // sideways
    else
        orientation = false;


    // waveform display upsampling
    setUps();

    // set color of rect
    randColor();
}


// other intialization code
void SampleVis::init()
{

    // selection state
    isSelected = false;
    buffMult = (double)1.0 / globalAtten;
    // other params
    showBuff = true;
    pendingBuffState = true;
    // alpha pulsation stuff
    aMin = 0.2f;
    aMax = 0.3f;
    aPhase = 2.0f * PI * ((float)rand() / RAND_MAX);
    lambda = 0.001;
    pRate = ((float)rand() / RAND_MAX) * 0.001;

    if (((float)rand() / RAND_MAX) > 0.5)
        aTarg = aMin;

    else
        aTarg = aMax;

    buffAlphaMax = 0.75f;
    buffAlpha = buffAlphaMax;
    myBuff = NULL;
    myBuffFrames = 0;
    myBuffChans = 0;
    myBuffInc = 0;
    lastX = 0;
    lastY = 0;

    // get start time
    startTime = GTime::instance().sec;
    // set id
    // myId = ++boxId;
}


// set selection (highlight)
void SampleVis::setSelectState(bool state)
{
    isSelected = state;
}
// determine if mouse click is in selection range
bool SampleVis::select(float x, float y)
{

    // check selection
    bool isInside = insideMe(x, y);
    // cout << "x " << x << "  y " << y << endl;
    //    cout << "rleft " << rleft << "rright " << rright << "rtop " << rtop << "rbot " << rbot  << endl;

    // if selected set selection position (center, top, right, bottom, left)

    // return selection state
    return isInside;
}

// process mouse motion during selection
void SampleVis::move(float xDiff, float yDiff)
{
    rX = rX + xDiff;
    rY = rY + yDiff;
    updateCorners(rWidth, rHeight);
}

void SampleVis::setXY(float x, float y)
{
    rX = x;
    rY = y;
    updateCorners(rWidth, rHeight);
}

bool SampleVis::getOrientation()
{
    return orientation;
}

void SampleVis::toggleOrientation()
{
    orientation = !orientation;
    setWidthHeight(rHeight, rWidth);
}


// color randomizer + alpha (roughly green/blue in color)
void SampleVis::randColor()
{
    // color
    colR = 0.4 + ((float)rand() / RAND_MAX) * 0.3;
    colG = colR;
    colB = colR;
    // colG = 0.39f + ((float)rand()/RAND_MAX)*0.51f;
    // colB = 0.27f + ((float)rand()/RAND_MAX)*0.63f;
    colA = aMin + ((float)rand() / RAND_MAX) * 0.2f;
    // wPulse = 0.95 + ((float)rand()/RAND_MAX) *0.1;
}

// set width and height
void SampleVis::setWidthHeight(float width, float height)
{


    bool newWidth = false;
    bool newHeight = false;
    if (width >= minDim) {
        rWidth = width;
        newWidth = true;
    }

    if (height >= minDim) {
        rHeight = height;
        newHeight = true;
    }

    if (newWidth || newHeight) {
        updateCorners(rWidth, rHeight);

        // update waveform params
        setUps();
        setWaveDisplayParams();
    }


    /*if ((width > 50.0f) && (width < (8.0f*screenWidth))){
        rWidth = width;
        newSet = true;
    }
    if ( (height > 50.0f) && (width < (8.0f*screenHeight))){
        rHeight = height;
        newSet = true;
    }

    if (newSet == true){
        updateCorners(rWidth,rHeight);

        //update waveform params
        setUps();
        setWaveDisplayParams();
    }
     */
}

// getters for width and height
float SampleVis::getWidth()
{
    return rWidth;
}

float SampleVis::getHeight()
{
    return rHeight;
}

// getters for X and Y
float SampleVis::getX()
{
    return rX;
}

float SampleVis::getY()
{
    return rY;
}

// update box corners with new width values
void SampleVis::updateCorners(float width, float height)
{
    rtop = rY + height * 0.5f;
    rbot = rY - height * 0.5f;
    rright = rX + width * 0.5f;
    rleft = rX - width * 0.5f;
}


// set upsampling for waveform display
void SampleVis::setUps()
{
    // get screen width and height
    MyGLScreen *screen = theApplication->GLwindow()->screen();
    unsigned screenWidth = screen->width();
    unsigned screenHeight = screen->height();

    //float sizeFactor = 10.0f;
    if (orientation == true)
        ups = (float)screenWidth / rWidth;
    else
        ups = (float)screenHeight / rHeight;

    if (ups < 1)
        ups = 1;
}


////process mouse movement/selection
// void SampleVis::procMovement(int x, int y)
//{
//    for (int i = 0; i<5; i++){
//        if (pickedArray[i] == 1){
//            cout << "pickedArray[" << i <<"]: " << pickedArray[i]<<endl;
//            switch (i) {
//                case INSIDE:
//                    if (lastX != (float)x){
//                        rX += ((float)x - lastX);
//                        updateCorners(rWidth,rHeight);
//                    }
//                    if (lastY != (float)y){
//                        rY -= ((float)y - lastY);
//                        updateCorners(rWidth,rHeight);
//                    }
//                    //                    if (lastY > 0){
//                    //                        rHeight -= (y- lastY);
//                    //                        updateCorners(rWidth, rHeight);
//                    //                    }
//                    //                    // m_top = (m_screenHeight - y) *
//                    m_screenScaleH; break;
//                case TOP:
//                    //m_bottom = (m_screenHeight - y) * m_screenScaleH;
//                    break;
//                case BOTTOM:
//                    //m_right = x * m_screenScaleW;
//                    break;
//                case LEFT:
//                    // m_left = x * m_screenScaleW;
//                    break;
//                case RIGHT:
//                    if (lastX != (float)x){
//                        rX += (float)x - lastX;
//                        cout << "rx" << rX << endl;
//                        updateCorners(rWidth,rHeight);
//                    }
//                    if (lastY != (float)y){
//                        rY -= ((float)y - lastY);
//                        updateCorners(rWidth,rHeight);
//                    }
//                    break;
//
//                default:
//                    break;
//            }
//        }
//    }
//
//    lastX = (float)x;
//    lastY = (float)y;
//}

void SampleVis::associateSample(BUFFERPREC *theBuff, unsigned long buffFrames, unsigned int buffChans, const string &name)
{

    myBuff = theBuff;
    myBuffFrames = buffFrames;
    myBuffChans = buffChans;
    rName = name;
    //    if (orientation == true)
    //        setWidthHeight((float)buffFrames/20000.f,rHeight);
    //    else
    //        setWidthHeight(rWidth,(float)buffFrames/30000.f);

    setWaveDisplayParams();
}

void SampleVis::registerSample(Sample *sampleToRegister)
{
    mySample = sampleToRegister;
}

void SampleVis::setWaveDisplayParams()
{
    if (orientation == true) {
        myBuffInc = myBuffFrames / (ups * rWidth);
    }
    else {
        myBuffInc = myBuffFrames / (ups * rHeight);
    }
}

// draw function
void SampleVis::draw()
{
    // get screen width and height
    MyGLScreen *screen = theApplication->GLwindow()->screen();
    unsigned screenWidth = screen->width();
    unsigned screenHeight = screen->height();

    //glPushMatrix();

    // rect properties
    //
    //    if (colA > (aMax - 0.005)){
    //        aTarg = aMin;
    //        lambda = 0.999+pRate;
    //    }else if (colA < (aMin + 0.005))
    //    {
    //        aTarg = aMax;
    //        lambda = 0.999;
    //    }
    //
    // colA = 0.18f + 0.1f*sin(0.5f*PI*GTime::instance().sec + aPhase);

    // draw rectangle
    glColor4f(colR, colG, colB, colA);

    //    double theTime = (GTime::instance().sec-startTime);
    //    rX += 0.1*sin(2*PI*0.12*theTime);
    //    rY += 0.5*sin(2*PI*0.1*theTime);
    //    updateCorners(rWidth,rHeight);
    //
    glBegin(GL_QUADS);
    glVertex3f(rleft, rtop, 0.0f);
    glVertex3f(rright, rtop, 0.0f);
    glVertex3f(rright, rbot, 0.0f);
    glVertex3f(rleft, rbot, 0.0f);
    glEnd();


    if (isSelected == true) {
        glColor4f(0.1, 0.7, 0.6, 0.35);
        glLineWidth(2.0f);
        glBegin(GL_LINE_STRIP);
        glVertex3f(rleft, rtop, 0.0f);
        glVertex3f(rright, rtop, 0.0f);
        glVertex3f(rright, rbot, 0.0f);
        glVertex3f(rleft, rbot, 0.0f);
        glVertex3f(rleft, rtop, 0.0f);
        glEnd();
    }

    // draw audio buffer
    if ((myBuff) && ((showBuff == true) || (pendingBuffState == true))) {
        //cout << "affiche sample" << endl;        // fade in out waveform
        if (pendingBuffState == false) {
            buffAlpha = 0.996 * buffAlpha;
            if (buffAlpha < 0.001)
                showBuff = false;
        }
        else {
            buffAlpha = 0.996 * buffAlpha + 0.004 * buffAlphaMax;
        }

        // buffMult = (buffAlpha + 0.35)/globalAtten;;

        glLineWidth(0.3);
        float waveCol = 1.0f;
        glColor4f(waveCol, waveCol, waveCol, colA * buffAlpha);
        glPointSize(1.0);
        switch (myBuffChans) {
        case 1: {
            glBegin(GL_LINE_STRIP);
            double waveAmplitude = 0;
            int ptrWave = 0;
            if (orientation == true) {
                for (int i = 0; i < rWidth * ups; i++) {
                    float nextI = (float)i / ups;
                    // circular buffer for inputs
                    ptrWave = (int)floor(i * myBuffInc) + mySample->ptrLagWave;
                    if (ptrWave > mySample->frames - 1)
                        ptrWave = ptrWave - mySample->frames;
                    waveAmplitude = (myBuff[ptrWave]);
                    glVertex3f((rleft + nextI),
                               rY + 0.5f * rHeight * waveAmplitude,
                               0.0f);
                }
            }
            else {
                for (int i = 0; i < rHeight * ups; i++) {
                    float nextI = (float)i / ups;
                    // circular buffer for inputs
                    ptrWave = (int)floor(i * myBuffInc) + mySample->ptrLagWave;
                    if (ptrWave > mySample->frames - 1)
                        ptrWave = ptrWave - mySample->frames;
                    waveAmplitude = (myBuff[ptrWave]);
                    glVertex3f(rX + 0.5f * rWidth * waveAmplitude,
                               (rbot + nextI), 0.0f);
                }
            }
            glEnd();
            break;
        }
        case 2:
            // left channel
            if (orientation == true) {
                glBegin(GL_LINE_STRIP);
                for (int i = 0; i < rWidth * ups; i++) {
                    float nextI = (float)i / ups;
                    // glVertex3f((rleft + nextI),rY + rHeight*(0.24f * myBuff[2*(int)floor(i*myBuffInc)]*buffMult+0.1f),0.0f);
                    glVertex3f((rleft + nextI),
                               rY + 0.25 * rHeight +
                                   0.25f * rHeight * (myBuff[2 * (int)floor(i * myBuffInc)]),
                               0.0f);
                }

                glEnd();

                // right channel
                glBegin(GL_LINE_STRIP);

                for (int i = 0; i < rWidth * ups; i++) {
                    float nextI = (float)i / ups;

                    glVertex3f((rleft + nextI),
                               rY - 0.25 * rHeight +
                                   0.25f * rHeight *
                                       (myBuff[2 * (int)floor(i * myBuffInc) + 1]),
                               0.0f);
                }
                glEnd();
            }
            else {
                glBegin(GL_LINE_STRIP);
                for (int i = 0; i < rHeight * ups; i++) {
                    float nextI = (float)i / ups;

                    // glVertex3f(rX + rWidth*(0.24f * myBuff[2*(int)floor(i*myBuffInc)]*buffMult+0.1f),(rbot + nextI),0.0f);
                    glVertex3f(rX + 0.25 * rWidth +
                                   0.25f * rWidth * (myBuff[2 * (int)floor(i * myBuffInc)]),
                               (rbot + nextI), 0.0f);
                }

                glEnd();

                // right channel
                glBegin(GL_LINE_STRIP);

                for (int i = 0; i < rHeight * ups; i++) {
                    float nextI = (float)i / ups;

                    //   glVertex3f(rX + rWidth*(0.24f * myBuff[2*(int)floor(i*myBuffInc)+1]-0.1f),(rbot + nextI),0.0f);
                    glVertex3f(rX - 0.25 * rWidth +
                                   0.25f * rWidth *
                                       (myBuff[2 * (int)floor(i * myBuffInc) + 1]),
                               (rbot + nextI), 0.0f);
                }
                glEnd();
            }
        default:
            break;
        }
    }

    if (showSampleNames) {
        glColor4f(1.0, 0.0, 0.0, 1.0);
        QString text = QString::fromStdString(rName);
        draw_string(rleft, screenHeight - rtop, 0.5f, text, 80.0f);
    }

    //glPopMatrix();
}


void SampleVis::toggleWaveDisplay()
{
    pendingBuffState = !pendingBuffState;
    if (pendingBuffState == true)
        showBuff = true;
}

// return id
// unsigned int SampleVis::getId()
//{
//    return myId;
//}


// check to see if a coordinate is inside this rectangle
bool SampleVis::insideMe(float x, float y)
{
    if ((x > rleft) && (x < rright)) {
        if ((y > rbot) && (y < rtop)) {
            return true;
        }
    }
    return false;
}

// return normalized position values in x and y
bool SampleVis::getNormedPosition(double *positionsX, double *positionsY,
                                  float x, float y, unsigned int idx)
{
    bool trigger = false;
    // cout << "grainX:  " << x << " grainY: " << y << " rleft " << rleft << " rright" << rright << " rtop " << rtop << " rbottom " << rbot <<  endl;
    if (insideMe(x, y) == true) {
        trigger = true;
        if (orientation == true) {
            positionsX[idx] = (double)((x - rleft) / rWidth);
            positionsY[idx] = (double)((y - rbot) / rHeight);
        }
        else {
            positionsY[idx] = (double)((x - rleft) / rWidth);
            positionsX[idx] = (double)((y - rbot) / rHeight);
        }
        // cout << positionsX[idx] << ", " << positionsY[idx]<<endl;
        if ((positionsY[idx] < 0.0) || (positionsY[idx] > 1.0))
            cout << "problem with x trigger pos - see sampleVis get normed pos" << endl;
        if ((positionsY[idx] < 0.0) || (positionsY[idx] > 1.0))
            cout << "problem with x trigger pos - see sampleVis get normed pos" << endl;
    }
    return trigger;
}


// set name
void SampleVis::setName(const string &name)
{
    rName = name;
    return;
}

// print information
void SampleVis::describe(std::ostream &out)
{
    out << "- orientation : " << getOrientation() << "\n";
    out << "- height : " << getHeight() << "\n";
    out << "- width : " << getWidth() << "\n";
    out << "- X : " << getX() << "\n";
    out << "- Y : " << getY() << "\n";
}

const string &SampleVis::getName() const
{
    return rName;
}
