#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
#include "cinder/ObjLoader.h"
#include "cinder/params/Params.h"
#include "cinder/Rand.h"
#include "cinder/Text.h"
#include "cinder/Font.h"

#include "boost/function.hpp"
#include "boost/bind.hpp"
#include "boost/lambda/lambda.hpp"
#include "boost/python.hpp"

#include "DeferredRenderer.h"
#include "Resources.h"

#include <chrono>

using namespace ci;
using namespace ci::app;
using namespace std;


static const float	APP_RES_HORIZONTAL = 1024.0f;
static const float	APP_RES_VERTICAL = 768.0f;
static const Vec3f	CAM_POSITION_INIT( -24.0f, 7.0f, -24.0f );
static const Vec3f	LIGHT_POSITION_INIT( 3.0f, 1.5f, 0.0f );

struct AzmAlt {
  double azm;
  double alt;
};

class LandscapeApp : public AppBasic
{
  public:
    void prepareSettings( Settings *settings );
    void setup();
    void update();
    void draw();
    void shutdown();
    void keyDown( app::KeyEvent event );
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
  
  private:
  
    // renderer
    void drawShadowCasters(gl::GlslProg* deferShader) const;
    void drawNonShadowCasters(gl::GlslProg* deferShader) const;
    void drawOverlay() const;
  
    //debug
    cinder::params::InterfaceGl mParams;
    int RENDER_MODE;
    bool mShowParams;
    float	mCurrFramerate;
    
    //camera
    MayaCamUI mMayaCam;
    CameraPersp mCam;
    
    // renderer
    DeferredRenderer mDeferredRenderer;
    int mCurrLightIndex;
    
    // object loading
    TriMesh mMesh;
    gl::VboMesh mVBO;
    
    // light positions
    AzmAlt getSunPosition(double day);
    AzmAlt getMoonPosition(double day);
    
    // python
    PyObject* py_ephem;
    PyObject* py_em;
  
    // time
    double mTimeDay;
    double getNowDay();
//    string getNowString();
  
    // colors
    float mAmbientBrightness;
    ColorA mSunColor;
    ColorA mMoonColor;
  
    // positions
    AzmAlt mSunPos;
    AzmAlt mMoonPos;
  
};



void LandscapeApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize ( APP_RES_HORIZONTAL, APP_RES_VERTICAL );
  settings->setBorderless ( false );
	settings->setFrameRate  ( 1000.0f ); // max it out
	settings->setResizable  ( false ); // non-resizable because of the shadowmaps
  settings->setFullScreen ( false );
	// make sure secondary screen isn't blacked out as well when in fullscreen
	// settings->enableSecondaryDisplayBlanking( false );
}

void LandscapeApp::setup()
{

	gl::disableVerticalSync(); // for faster framerate
  
  // variable inits
	RENDER_MODE = DeferredRenderer::SHOW_FINAL_VIEW;
  mCurrFramerate  = 0.0f;
  mCurrLightIndex = 0;
  mSunColor  = Color(1.0f, 1.0f, 0.9f);
  mMoonColor = Color(0.24f, 0.15f, 0.94f);
  mAmbientBrightness = 100.0f;

  // load object and put it into a VBO
  ObjLoader loader( (DataSourceRef)loadResource( RES_LANDSCAPE_OBJ ) );
  loader.load( &mMesh );
  mVBO = gl::VboMesh( mMesh );
  
	//set up camera
	mCam.setPerspective( 45.0f, getWindowAspectRatio(), 0.1f, 10000.0f );
  mCam.lookAt(CAM_POSITION_INIT * 1.5f, Vec3f::zero(), Vec3f(0.0f, 1.0f, 0.0f) );
  mCam.setCenterOfInterestPoint(Vec3f::zero());
  mMayaCam.setCurrentCam(mCam);
  
  //create functions pointers to send to deferred renderer
  boost::function<void(gl::GlslProg*)> fRenderShadowCastersFunc =
    boost::bind( &LandscapeApp::drawShadowCasters, this, boost::lambda::_1 );
  boost::function<void(gl::GlslProg*)> fRenderNotShadowCastersFunc =
    boost::bind( &LandscapeApp::drawNonShadowCasters, this,  boost::lambda::_1 );
  boost::function<void(void)> fRenderOverlayFunc =
    boost::bind( &LandscapeApp::drawOverlay, this );
  
  // setup deferred renderer
  mDeferredRenderer.setup(
                          fRenderShadowCastersFunc,
                          fRenderNotShadowCastersFunc,
                          NULL, //fRenderOverlayFunc,
                          NULL,
                          &mCam,
                          Vec2i(APP_RES_HORIZONTAL, APP_RES_VERTICAL),
                          1024,
                          true,
                          true
                          );
  
  // add point lights
  mDeferredRenderer.addPointLight( Vec3f(0,0,0),
                                   mSunColor * mAmbientBrightness * 0.8,
                                   true,
                                   true);
  mDeferredRenderer.addPointLight( Vec3f(0,0,0),
                                   mMoonColor * mAmbientBrightness * 0.8,
                                   true,
                                   true);
  
  
  // init python
  Py_Initialize();
  // import ephemScript from /Library/Python/2.7/site-specific/
  py_ephem = PyImport_Import(PyString_FromString((char*)"ephem"));
  py_em = PyImport_Import(PyString_FromString((char*)"ephemScript"));
  
  mTimeDay = getNowDay();
  
  console() << "mTimeDay: " << mTimeDay << endl;
  
//  console() << "getSunPos: " << getSunPosition(mTimeHours) << endl;
}


double LandscapeApp::getNowDay() {
  PyObject* getNowDay = PyObject_GetAttrString(py_em,(char*)"getNowDay");
  return PyFloat_AsDouble(PyObject_CallObject(getNowDay, NULL));
}

//string LandscapeApp::getNowString() {
//  PyObject* getNowString = PyObject_GetAttrString(py_em,(char*)"getNowString");
//  return PyObject_CallObject((char *)getNowString, NULL);
//}


void LandscapeApp::shutdown()
{
  // end python
  Py_Finalize();
  console() << "done" << std::endl;
}


void LandscapeApp::update()
{
  mCam = mMayaCam.getCamera();
  mDeferredRenderer.mCam = &mCam;
	mCurrFramerate = getAverageFps();

  mTimeDay += 1.0 / 24.0 / 60.0;
  if (false) {
    mSunPos  = getSunPosition(mTimeDay);
    mMoonPos = getMoonPosition(mTimeDay);
  }
  
  float lightDistance = 14.0f;
  
  // rotate sun
  Vec3f sunPos(lightDistance, 0, 0);
  sunPos.rotate(Vec3f(0, 0, 1), mSunPos.alt );
  sunPos.rotate(Vec3f(0, 1, 0), mSunPos.azm );
  mDeferredRenderer.getCubeLightsRef()->at(0)->setPos(sunPos);
  mDeferredRenderer.getCubeLightsRef()->at(0)->setCol(mSunColor
                                              * mAmbientBrightness * 0.8);
  
  // rotate sun
  Vec3f moonPos(lightDistance, 0, 0);
  moonPos.rotate(Vec3f(0, 0, 1), mMoonPos.alt );
  moonPos.rotate(Vec3f(0, 1, 0), mMoonPos.azm );
  mDeferredRenderer.getCubeLightsRef()->at(1)->setPos(moonPos);
  mDeferredRenderer.getCubeLightsRef()->at(1)->setCol(mMoonColor
                                              * mAmbientBrightness * 0.8);
  
}

void LandscapeApp::draw()
{
  mDeferredRenderer.renderFullScreenQuad(RENDER_MODE);
}

void LandscapeApp::mouseDown( MouseEvent event )
{
    if( event.isAltDown() ) {
      mMayaCam.mouseDown( event.getPos() );
    }
}

void LandscapeApp::mouseDrag( MouseEvent event )
{
    if( event.isAltDown() ) {
      mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
    }
}

void LandscapeApp::drawShadowCasters(gl::GlslProg* deferShader) const
{
  gl::pushMatrices();
  gl::scale(1.3f, 1.3f, 1.3f);
  gl::draw( mVBO );
  gl::popMatrices();
}

void LandscapeApp::drawNonShadowCasters(gl::GlslProg* deferShader) const
{
    int size = 10;
    //a plane to capture shadows (though it won't cast any itself)
    glColor3ub(255, 255, 255);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex3i( size, 0,-size);
    glVertex3i(-size, 0,-size);
    glVertex3i(-size, 0, size);
    glVertex3i( size, 0, size);
    glEnd();
}

void LandscapeApp::drawOverlay() const
{
  /*
  Vec3f camUp, camRight;
  mCam.getBillboardVectors(&camRight, &camUp);
  
  //create text labels
  TextLayout layout1;
	layout1.clear( ColorA( 1.0f, 1.0f, 1.0f, 0.0f ) );
	layout1.setFont( Font( "Arial", 34 ) );
	layout1.setColor( ColorA( 255.0f/255.0f, 255.0f/255.0f, 8.0f/255.0f, 1.0f ) );
	layout1.addLine( to_string(getAverageFps()) ); //to_string is a c++11 function for conversions
	Surface8u rendered1 = layout1.render( true, false );
  gl::Texture fontTexture_FR = gl::Texture( rendered1 );
  
  //draw framerate
  fontTexture_FR.bind();
  gl::drawBillboard(Vec3f(-3.0f, 7.0f, 0.0f), Vec2f(fontTexture_FR.getWidth()/20.0f , fontTexture_FR.getHeight()/20.0f), 0, camRight, camUp);
  fontTexture_FR.unbind();
  */
}

void LandscapeApp::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
      //switch between render views
		case KeyEvent::KEY_0:
    {RENDER_MODE = DeferredRenderer::SHOW_FINAL_VIEW;}
			break;
		case KeyEvent::KEY_1:
    {RENDER_MODE = DeferredRenderer::SHOW_DIFFUSE_VIEW;}
			break;
		case KeyEvent::KEY_2:
    {RENDER_MODE = DeferredRenderer::SHOW_NORMALMAP_VIEW;}
			break;
		case KeyEvent::KEY_3:
    {RENDER_MODE = DeferredRenderer::SHOW_DEPTH_VIEW;}
			break;
    case KeyEvent::KEY_4:
    {RENDER_MODE = DeferredRenderer::SHOW_POSITION_VIEW;}
			break;
    case KeyEvent::KEY_5:
    {RENDER_MODE = DeferredRenderer::SHOW_ATTRIBUTE_VIEW;}
			break;
    case KeyEvent::KEY_6:
    {RENDER_MODE = DeferredRenderer::SHOW_SSAO_VIEW;}
			break;
    case KeyEvent::KEY_7:
    {RENDER_MODE = DeferredRenderer::SHOW_SSAO_BLURRED_VIEW;}
			break;
    case KeyEvent::KEY_8:
    {RENDER_MODE = DeferredRenderer::SHOW_LIGHT_VIEW;}
      break;
    case KeyEvent::KEY_9:
    {RENDER_MODE = DeferredRenderer::SHOW_SHADOWS_VIEW;}
			break;
      
    // change which cube you want to control
    case 269: {
      //minus key
      if( mDeferredRenderer.getNumCubeLights() > 0) {
        --mCurrLightIndex;
        if ( mCurrLightIndex < 0) mCurrLightIndex = mDeferredRenderer.getNumCubeLights() - 1;
      }
    }
      break;
    case 61: {
      if( mDeferredRenderer.getNumCubeLights() > 0) {
        //plus key
        ++mCurrLightIndex;
        if ( mCurrLightIndex > mDeferredRenderer.getNumCubeLights() - 1) mCurrLightIndex = 0;
      }
    }
      break;
			
		case KeyEvent::KEY_UP:     { } break;
		case KeyEvent::KEY_DOWN:   { } break;
		case KeyEvent::KEY_LEFT:   { } break;
		case KeyEvent::KEY_RIGHT:  { } break;
    case KeyEvent::KEY_ESCAPE: { exit(1); } break;
		default : break;
	}
}


AzmAlt LandscapeApp::getSunPosition(double day) {
  // get azimuth
  PyObject* getSunAzimuth = PyObject_GetAttrString(py_em, (char*)"getSunAzimuth");
  PyObject* args = PyTuple_Pack(1, PyFloat_FromDouble(day));
  PyObject* az = PyObject_CallObject(getSunAzimuth, args);
  // get altitude
  PyObject* getSunAltitude = PyObject_GetAttrString(py_em, (char*)"getSunAltitude");
  args = PyTuple_Pack(1, PyFloat_FromDouble(day));
  PyObject* alt = PyObject_CallObject(getSunAltitude, args);
  return { PyFloat_AsDouble(az), PyFloat_AsDouble(alt)};
}
AzmAlt LandscapeApp::getMoonPosition(double day) {
  // get azimuth
  PyObject* getMoonAzimuth = PyObject_GetAttrString(py_em, (char*)"getMoonAzimuth");
  PyObject* args = PyTuple_Pack(1, PyFloat_FromDouble(day));
  PyObject* az = PyObject_CallObject(getMoonAzimuth, args);
  // get altitude
  PyObject* getMoonAltitude = PyObject_GetAttrString(py_em, (char*)"getMoonAltitude");
  args = PyTuple_Pack(1, PyFloat_FromDouble(day));
  PyObject* alt = PyObject_CallObject(getMoonAltitude, args);
  return {PyFloat_AsDouble(az), PyFloat_AsDouble(alt)};
}


CINDER_APP_BASIC( LandscapeApp, RendererGl )
