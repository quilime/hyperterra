// cinder
#include "cinder/app/AppBasic.h"
#include "cinder/System.h"
#include "cinder/Filesystem.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
#include "cinder/ObjLoader.h"
#include "cinder/Rand.h"
#include "cinder/Text.h"
#include "cinder/Plane.h"
#include "cinder/Font.h"
#include "cinder/Json.h"


// boost
#include "boost/function.hpp"
#include "boost/bind.hpp"
#include "boost/lambda/lambda.hpp"
#include "boost/python.hpp"

// blocks
#include "DeferredRenderer.h"

// osc
namespace boost {
  typedef std::thread thread;
}

#include "OscListener.h"
#include "OscSender.h"

// resources
#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

static const float	APP_RES_HORIZONTAL = 1920.0f;
static const float	APP_RES_VERTICAL = 1200.0f;
static const float  DEFAULT_LIGHT_DIST = 14.f;

struct AzmAlt {
  double azm;
  double alt;
  double dist;
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
    void mouseMove( MouseEvent event );
    void mouseDrag( MouseEvent event );
    void mouseUp( MouseEvent event);
  
  private:
  
    // osc
    osc::Listener 	listener;
    osc::Sender     sender;
    std::string host;
    void updateOSC();
  
    // renderer
    void drawShadowCasters(gl::GlslProg* deferShader) const;
    void drawNonShadowCasters(gl::GlslProg* deferShader) const;
    int RENDER_MODE;
  
    // setup
    float	mCurrFramerate;
    bool mSetup, mBaseSetup;
    bool mVertexDragging;
    bool mMouseEdit, mMouseLMB, mMouseMMB, mMouseRMB;
    Vec2f mNearestVertex;
    vector<int> mNearestIndicies;
    int mNearestIndex;
    Vec2f mMousePos;
    vector<Vec3f> baseVerts;
  
    void loadSettings();
    void saveSettings();
  
    // vertex editing
    Vec3f screenToWorldAtVertexPlane(const Vec2f &screenPos, const Planef &p);
    Planef mDragVertexPlane;
    Vec3f mDragVertexOffset;
    vector<int> getCoincidentVertIndicies(const Vec3f &v, const vector<Vec3f> &verts);
  
    //camera
    MayaCamUI mMayaCam;
    CameraPersp mCam;
    
    // renderer
    DeferredRenderer mDeferredRenderer;
    
    // mesh
    TriMesh mMesh;
    gl::VboMeshRef mVBO;
    gl::VboMesh::Layout mLayout;
    
    // light positions
    AzmAlt getSunPosition(double day);
    AzmAlt getMoonPosition(double day);
    
    // python
    PyObject* py_ephem;
    PyObject* py_em;
  
    // place
    double mNorth;
  
    // time
    double mTimeDay;
    double mTimeIncrement;
    double mTimeTracking;
    double getNowDay();
    int mCount;
  
    // colors
    float mAmbientBrightness, mAmbientBrightnessMult;
    ColorA mSunColor;
    float mSunBrightness;
    ColorA mMoonColor;
    float mMoonBrightness;

  
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
//	settings->enableSecondaryDisplayBlanking( false );
//  settings->en
}


void LandscapeApp::setup()
{
  // init python
  Py_Initialize();
  // import ephemScript from /Library/Python/2.7/site-specific/
  py_ephem = PyImport_Import(PyString_FromString((char*)"ephem"));
  py_em = PyImport_Import(PyString_FromString((char*)"ephemScript"));
  
  // osc setup
  int port = 3000;
  listener.setup( port );
	host = "192.168.0.100"; //System::getIpAddress();
//	if( host.rfind( '.' ) != string::npos )
//		host.replace( host.rfind( '.' ) + 1, 3, "255" );
	sender.setup( host, port, true );
  
	gl::disableVerticalSync(); // for faster framerate
  
  // variable inits
	RENDER_MODE = DeferredRenderer::SHOW_FINAL_VIEW;
  mCurrFramerate  = 0.0f;
  mSunPos = getSunPosition(getNowDay());
  mMoonPos = getMoonPosition(getNowDay());
  mSunColor  = Color(1.0f, 1.0f, 0.9f);
  mMoonColor = Color(0.24f, 0.15f, 0.94f);
  mAmbientBrightnessMult = 1000.0f;
  mAmbientBrightness = 0.1f;
  mSunBrightness = 1.0f;
  mMoonBrightness = 1.0f;
  mCount = 0;
  mSetup = false;
  mBaseSetup = false;
  
  // set up base
  float size = 10;
  baseVerts.push_back(Vec3f(  size, 0, -size));
  baseVerts.push_back(Vec3f( -size, 0, -size));
  baseVerts.push_back(Vec3f( -size, 0,  size));
  baseVerts.push_back(Vec3f(  size, 0,  size));
  

  // load object and put it into a VBO
  ObjLoader loader( (DataSourceRef)loadAsset( RES_LANDSCAPE_OBJ ) );
  loader.load( &mMesh );
  mLayout.setDynamicPositions();
  mLayout.setDynamicNormals();
  mLayout.setDynamicIndices();
  mVBO = gl::VboMesh::create( mMesh, mLayout );
  
	//set up camera
	mCam.setPerspective( 45.0f, getWindowAspectRatio(), 0.1f, 10000.0f );
  mCam.lookAt(Vec3f( -20.0f, 15.0f, -20.0f ), Vec3f::zero(), Vec3f(0.0f, 1.0f, 0.0f) );
  mCam.setCenterOfInterestPoint(Vec3f::zero());
  mMayaCam.setCurrentCam(mCam);
  
  //create functions pointers to send to deferred renderer
  boost::function<void(gl::GlslProg*)> fRenderShadowCastersFunc =
    boost::bind( &LandscapeApp::drawShadowCasters, this, boost::lambda::_1 );
  boost::function<void(gl::GlslProg*)> fRenderNotShadowCastersFunc =
    boost::bind( &LandscapeApp::drawNonShadowCasters, this,  boost::lambda::_1 );

  // setup deferred renderer
  mDeferredRenderer.setup(
                          fRenderShadowCastersFunc,
                          fRenderNotShadowCastersFunc,
                          NULL,//fRenderOverlayFunc, // NULL
                          NULL,
                          &mCam,
                          Vec2i(APP_RES_HORIZONTAL, APP_RES_VERTICAL),
                          1024,
                          true,
                          true
                          );
  
  // add point lights
  mDeferredRenderer.addPointLight( Vec3f(0,0,0),
                                   mSunColor * 0.8 *
                                  (mAmbientBrightness * mAmbientBrightnessMult),
                                   true,
                                   false);
  mDeferredRenderer.addPointLight( Vec3f(0,0,0),
                                   mMoonColor * 0.8 *
                                  (mAmbientBrightness * mAmbientBrightnessMult),
                                   false,
                                   false);
  
  // follow time right now as a float in days
  mTimeTracking = true;
  mTimeIncrement = 0;
  mNorth = 0;
  mTimeDay = getNowDay();
}


double LandscapeApp::getNowDay() {
  PyObject* getNowDay = PyObject_GetAttrString(py_ephem, (char*)"now");
  return PyFloat_AsDouble(PyObject_CallObject(getNowDay, NULL));
}


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
  
  updateOSC();

  if (mTimeTracking) {
    mTimeDay += mTimeIncrement;
    AzmAlt curSunPos = getSunPosition(mTimeDay);
    mSunPos.azm = curSunPos.azm;
    mSunPos.alt = curSunPos.alt;
    AzmAlt curMoonPos = getMoonPosition(mTimeDay);
    mMoonPos.azm = curMoonPos.azm;
    mMoonPos.alt = curMoonPos.alt;
  }
  
  // brightness based on if the bodies dip below the horizon
//  mSunBrightness = mSunPos.alt > 0 ? 1 : 0;
//  mMoonBrightness = mMoonPos.alt > 0 ? 1 : 0;
  
  
  // rotate sun
  Vec3f sunPos(mSunPos.dist, 0, 0);
  sunPos.rotate(Vec3f(0, 0, 1), mSunPos.alt );
  sunPos.rotate(Vec3f(0, 1, 0), mSunPos.azm + mNorth );
  mDeferredRenderer.getCubeLightsRef()->at(0)->setPos(sunPos);
  mDeferredRenderer.getCubeLightsRef()->at(0)->setCol(mSunColor *
                                                      (mAmbientBrightness *
                                                      mAmbientBrightnessMult) *
                                                      mSunColor.a *
                                                      mSunBrightness);
  // rotate sun
  Vec3f moonPos(mMoonPos.dist, 0, 0);
  moonPos.rotate(Vec3f(0, 0, 1), mMoonPos.alt );
  moonPos.rotate(Vec3f(0, 1, 0), mMoonPos.azm + mNorth );
  mDeferredRenderer.getCubeLightsRef()->at(1)->setPos(moonPos);
  mDeferredRenderer.getCubeLightsRef()->at(1)->setCol(mMoonColor *
                                                      (mAmbientBrightness *
                                                      mAmbientBrightnessMult) *
                                                      mMoonColor.a *
                                                      mMoonBrightness);
}

void LandscapeApp::updateOSC()
{
  while( listener.hasWaitingMessages() ) {
		osc::Message message;
		listener.getNextMessage( &message );
    
    float pr = false;
    
    if (pr) {
      console() << "New message received" << std::endl;
      console() << "Address: " << message.getAddress() << std::endl;
      console() << "Num Arg: " << message.getNumArgs() << std::endl;
    }
		for (int i = 0; i < message.getNumArgs(); i++) {
      if (pr) {
        console() << "-- Argument " << i << std::endl;
        console() << "---- type: " << message.getArgTypeName(i) << std::endl;
      }
			if( message.getArgType(i) == osc::TYPE_INT32 ) {
				try {
          if (pr)
            console() << "------ value: "<< message.getArgAsInt32(i) << std::endl;
				}
				catch (...) {
					console() << "Exception reading argument as int32" << std::endl;
				}
			}
			else if( message.getArgType(i) == osc::TYPE_FLOAT ) {
				try {
          if (pr)
            console() << "------ value: " << message.getArgAsFloat(i) << std::endl;
				}
				catch (...) {
					console() << "Exception reading argument as float" << std::endl;
				}
			}
			else if( message.getArgType(i) == osc::TYPE_STRING) {
				try {
          if (pr)
            console() << "------ value: " << message.getArgAsString(i).c_str() << std::endl;
				}
				catch (...) {
					console() << "Exception reading argument as string" << std::endl;
				}
			}
		}
    
    if( message.getNumArgs() != 0 && message.getArgType( 0 ) == osc::TYPE_FLOAT){
      
      // settings
      if (message.getAddress() == "/settings/setup") {
        mSetup = message.getArgAsFloat(0) == 0 ? false : true ;
      }
      if (message.getAddress() == "/settings/base") {
        mBaseSetup = message.getArgAsFloat(0) == 0 ? false : true ;
      }
      if (message.getAddress() == "/settings/save") {
        if (message.getArgAsFloat(0) == 1) {
          saveSettings();
        }
      }
      if (message.getAddress() == "/settings/load") {
        if (message.getArgAsFloat(0) == 1) {
          loadSettings();
        }
      }
      if (message.getAddress() == "/settings/rendermode") {
        int m = (int)message.getArgAsFloat(0);
        switch (m) {
          case 0: RENDER_MODE = DeferredRenderer::SHOW_FINAL_VIEW; break;
          case 1: RENDER_MODE = DeferredRenderer::SHOW_NORMALMAP_VIEW; break;
          case 2: RENDER_MODE = DeferredRenderer::SHOW_SHADOWS_VIEW; break;
        }
      }
      // mouse
      if (message.getAddress() == "/mouse/edit") {
        mMouseEdit = message.getArgAsFloat(0) == 0 ? false : true ;
      }
      if (message.getAddress() == "/mouse/lmb") {
        mMouseLMB = message.getArgAsFloat(0) == 0 ? false : true ;
      }
      if (message.getAddress() == "/mouse/mmb") {
        mMouseMMB = message.getArgAsFloat(0) == 0 ? false : true ;
      }
      if (message.getAddress() == "/mouse/rmb") {
        mMouseRMB = message.getArgAsFloat(0) == 0 ? false : true ;
      }                  
      

      // time
      if (message.getAddress() == "/time/reset") {
        mTimeDay = getNowDay();
        console() << "Now day; " << mTimeDay << endl;
        mTimeIncrement = 0;
      }
      if (message.getAddress() == "/time/tracking") {
        mTimeTracking = message.getArgAsFloat(0) == 0 ? false : true;
      }
      if (message.getAddress() == "/time/inc") {
        // subtracting here for some reason? weird.
        mTimeIncrement = -message.getArgAsFloat(0) / 24.0 / 60.0;
      }
      if (message.getAddress() == "/time/day") {
        mTimeDay = message.getArgAsFloat(0);
      }
      
      // ambient
      if (message.getAddress() == "/ambient/brightness") {
        mAmbientBrightness = message.getArgAsFloat(0);
      }
      
      // north
      if (message.getAddress() == "/north") {
        mNorth = message.getArgAsFloat(0) * (M_PI/180.0f) ;
      }
      
      // sun color
      if (message.getAddress() == "/sun/r") {
        mSunColor.r = message.getArgAsFloat(0);
      }
      if (message.getAddress() == "/sun/g") {
        mSunColor.g = message.getArgAsFloat(0);
      }
      if (message.getAddress() == "/sun/b") {
        mSunColor.b = message.getArgAsFloat(0);
      }
      if (message.getAddress() == "/sun/a") {
        mSunColor.a = message.getArgAsFloat(0);
      }
      
      // moon color
      if (message.getAddress() == "/moon/r") {
        mMoonColor.r = message.getArgAsFloat(0);
      }
      if (message.getAddress() == "/moon/g") {
        mMoonColor.g = message.getArgAsFloat(0);
      }
      if (message.getAddress() == "/moon/b") {
        mMoonColor.b = message.getArgAsFloat(0);
      }
      if (message.getAddress() == "/moon/a") {
        mMoonColor.a = message.getArgAsFloat(0);
      }
      
      // sun position
      if (message.getAddress() == "/sun/azm") {
        mSunPos.azm = message.getArgAsFloat(0) * (M_PI/180.0f);
      }
      if (message.getAddress() == "/sun/alt") {
        mSunPos.alt = message.getArgAsFloat(0) * (M_PI/180.0f);
      }
      if (message.getAddress() == "/sun/dist") {
        mSunPos.dist = message.getArgAsFloat(0);
      }
      if (message.getAddress() == "/sun/reset") {
        double t = getNowDay();
        mSunPos.dist = DEFAULT_LIGHT_DIST;
        mSunPos.azm  = getSunPosition(t).azm;
        mSunPos.alt  = getSunPosition(t).alt;
      }
      
      
      // moon position
      if (message.getAddress() == "/moon/azm") {
        mMoonPos.azm = message.getArgAsFloat(0) * (M_PI/180.0f);
      }
      if (message.getAddress() == "/moon/alt") {
        mMoonPos.alt = message.getArgAsFloat(0) * (M_PI/180.0f);
      }
      if (message.getAddress() == "/moon/dist") {
        mMoonPos.dist = message.getArgAsFloat(0);
      }
      if (message.getAddress() == "/moon/reset") {
        double t = getNowDay();
        mMoonPos.dist = DEFAULT_LIGHT_DIST;
        mMoonPos.azm  = getMoonPosition(t).azm;
        mMoonPos.alt  = getMoonPosition(t).alt;
      }
      
      
    }
	}
}


void LandscapeApp::loadSettings(){

  // load camera settings
  JsonTree json( loadAsset( "settings.json" ) );
  mCam.setEyePoint(Vec3f(
                         json.getValueForKey<float>("camera_eye_point_x"),
                         json.getValueForKey<float>("camera_eye_point_y"),
                         json.getValueForKey<float>("camera_eye_point_z")));
  mCam.setViewDirection(Vec3f(
                         json.getValueForKey<float>("camera_viewdir_x"),
                         json.getValueForKey<float>("camera_viewdir_y"),
                         json.getValueForKey<float>("camera_viewdir_z")));
	mCam.setPerspective( json.getValueForKey<float>("camera_fov"), getWindowAspectRatio(), 0.1f, 10000.0f );
  mCam.setWorldUp(Vec3f(0.0f, 1.0f, 0.0f));
  mMayaCam.setCurrentCam(mCam);
  
  // set base
  baseVerts[0].set(json.getValueForKey<float>("base0x"),
                   json.getValueForKey<float>("base0y"),
                   json.getValueForKey<float>("base0z"));
  baseVerts[1].set(json.getValueForKey<float>("base1x"),
                   json.getValueForKey<float>("base1y"),
                   json.getValueForKey<float>("base1z"));
  baseVerts[2].set(json.getValueForKey<float>("base2x"),
                   json.getValueForKey<float>("base2y"),
                   json.getValueForKey<float>("base2z"));
  baseVerts[3].set(json.getValueForKey<float>("base3x"),
                   json.getValueForKey<float>("base3y"),
                   json.getValueForKey<float>("base3z"));
  
  // load object from assets folder
  ObjLoader loader( (DataSourceRef) loadAsset("tweaked.obj") );
  loader.load( &mMesh );
  mVBO = gl::VboMesh::create( mMesh, mLayout );
}


void LandscapeApp::saveSettings() {

  // save camera settings
  JsonTree json;
  json.addChild(JsonTree("camera_eye_point_x", mCam.getEyePoint().x));
  json.addChild(JsonTree("camera_eye_point_y", mCam.getEyePoint().y));
  json.addChild(JsonTree("camera_eye_point_z", mCam.getEyePoint().z));
  json.addChild(JsonTree("camera_viewdir_x", mCam.getViewDirection().x));
  json.addChild(JsonTree("camera_viewdir_y", mCam.getViewDirection().y));
  json.addChild(JsonTree("camera_viewdir_z", mCam.getViewDirection().z));
  json.addChild(JsonTree("camera_fov", mCam.getFov()));

  // base settings
  json.addChild(JsonTree("base0x", baseVerts[0].x));
  json.addChild(JsonTree("base0y", baseVerts[0].y));
  json.addChild(JsonTree("base0z", baseVerts[0].z));
  json.addChild(JsonTree("base1x", baseVerts[1].x));
  json.addChild(JsonTree("base1y", baseVerts[1].y));
  json.addChild(JsonTree("base1z", baseVerts[1].z));
  json.addChild(JsonTree("base2x", baseVerts[2].x));
  json.addChild(JsonTree("base2y", baseVerts[2].y));
  json.addChild(JsonTree("base2z", baseVerts[2].z));
  json.addChild(JsonTree("base3x", baseVerts[3].x));
  json.addChild(JsonTree("base3y", baseVerts[3].y));
  json.addChild(JsonTree("base3z", baseVerts[3].z));
  
  std::ofstream jsonOutStream( "assets/settings.json" );
  jsonOutStream << json.serialize();
  jsonOutStream.close();

  
  // write object out to assets folder
  // console() << getAssetPath("") / "tweaked.obj" << endl;
  // Cinder way is broken on Linux, DataAssetRef was acting up
  // ObjLoader::write(writeFile( "assets/tweaked.obj"), mMesh);

  // open stream
  console() << "saving obj" << endl;
  bool writeNormals = true;
  bool includeUVs = true;
  std::ofstream meshOutStream( "assets/tweaked.obj" );
  const size_t numVerts = mMesh.getNumVertices();
  for( size_t p = 0; p < numVerts; ++p ) {
    ostringstream os;
    os << "v " << mMesh.getVertices()[p].x << " " << mMesh.getVertices()[p].y << " " << mMesh.getVertices()[p].z << std::endl;
    meshOutStream << os.str().c_str();
  }

  const bool processTexCoords = mMesh.hasTexCoords() && includeUVs;
  if( processTexCoords ) {
    for( size_t p = 0; p < numVerts; ++p ) {
      ostringstream os;
      os << "vt " << mMesh.getTexCoords()[p].x << " " << mMesh.getTexCoords()[p].y << std::endl;
      meshOutStream << os.str().c_str();
    }
  }
  
  const bool processNormals = mMesh.hasNormals() && writeNormals;
  if( processNormals ) {
    for( size_t p = 0; p < numVerts; ++p ) {
      ostringstream os;
      os << "vn " << mMesh.getNormals()[p].x << " " << mMesh.getNormals()[p].y << " " << mMesh.getNormals()[p].z << std::endl;
      meshOutStream << os.str().c_str();
    }
  }
  
  const size_t numTriangles = mMesh.getNumTriangles();
  const std::vector<uint32_t>& indices( mMesh.getIndices() );
  for( size_t t = 0; t < numTriangles; ++t ) {
    ostringstream os;
    os << "f ";
    if( processNormals && processTexCoords ) {
      os << indices[t*3+0]+1 << "/" << indices[t*3+0]+1 << "/" << indices[t*3+0]+1 << " ";
      os << indices[t*3+1]+1 << "/" << indices[t*3+1]+1 << "/" << indices[t*3+1]+1 << " ";
      os << indices[t*3+2]+1 << "/" << indices[t*3+2]+1 << "/" << indices[t*3+2]+1 << " ";
    }
    else if ( processNormals ) {
      os << indices[t*3+0]+1 << "//" << indices[t*3+0]+1 << " ";
      os << indices[t*3+1]+1 << "//" << indices[t*3+1]+1 << " ";
      os << indices[t*3+2]+1 << "//" << indices[t*3+2]+1 << " ";
    }
    else if( processTexCoords ) {
      os << indices[t*3+0]+1 << "/" << indices[t*3+0]+1 << " ";
      os << indices[t*3+1]+1 << "/" << indices[t*3+1]+1 << " ";
      os << indices[t*3+2]+1 << "/" << indices[t*3+2]+1 << " ";
    }
    else { // just verts
      os << indices[t*3+0]+1 << " ";
      os << indices[t*3+1]+1 << " ";
      os << indices[t*3+2]+1 << " ";      
    }
    os << std::endl;
    meshOutStream << os.str().c_str();
  }  

  // close stream
  meshOutStream.close();
}


void LandscapeApp::draw()
{
  mDeferredRenderer.renderFullScreenQuad(RENDER_MODE);
  
  if (mSetup) {
    // use mesh verts unless base setup is enabled
    auto verts = mMesh.getVertices();
    if (mBaseSetup) {
      verts = baseVerts;
    }
    if (!mVertexDragging) {
      // loop through all verticies in mesh and find the nearest vert position and index
      float nearestDistance = 0;
      for(int i = 0; i < verts.size(); i++) {
        auto w = mCam.worldToScreen(verts.at(i), getWindowWidth(), getWindowHeight());
        float distance = w.distance(mMousePos);
        if( i == 0 || distance < nearestDistance) {
          nearestDistance = distance;
          mNearestVertex = w;
          mNearestIndex = i;
          mNearestIndicies = getCoincidentVertIndicies(verts[mNearestIndex], verts);
        }
      }
    }
    
    // draw a line from the nearest vertex to the mouse position
    gl::color(0.5, 0.5, 0.5);
    gl::lineWidth(1);
    gl::drawLine(mNearestVertex, mMousePos);
    
    // draw a cirle around the nearest vertex
    gl::color(1, 0, 1);
    gl::lineWidth(2);
    if (mVertexDragging) {
      gl::drawStrokedEllipse(mMousePos,  4, 4);
    } else {
      gl::drawStrokedEllipse(mNearestVertex, 4, 4);
    }
    
    // draw all points
    glPointSize(2);
    gl::color(1, 1, 1);
    gl::begin(GL_POINTS);
    for (int i = 0; i < verts.size(); i++) {
      gl::vertex(mCam.worldToScreen(verts.at(i), getWindowWidth(), getWindowHeight()));
    }
    gl::end();
  }
  
  gl::color(1, 1, 1);
}


vector<int> LandscapeApp::getCoincidentVertIndicies(const Vec3f &v,
                                                   const vector<Vec3f> &verts ){
  vector<int> coincident;
  int c = 0;
  for (auto &vert : verts) {
    if (vert == v) {
      coincident.push_back(c);
    }
    c++;
  }
  return coincident;
}


Vec3f LandscapeApp::screenToWorldAtVertexPlane(const Vec2f &screenPos, const Planef &p)
{
  float t;
  auto uv = screenPos / getWindowSize();
  Ray r = mCam.generateRay(uv.x, 1-uv.y, getWindowAspectRatio() );
  bool hit = r.calcPlaneIntersection(p.getPoint(), p.getNormal(), &t);
  assert(hit); // Should always hit.
  return r.calcPosition(t);
}


void LandscapeApp::mouseDown( MouseEvent event )
{
  if( mMouseEdit ) {
    mMayaCam.mouseDown( event.getPos() );
  }
  else {
    if (mSetup) {
      auto v = mMesh.getVertices()[mNearestIndex];
      if (mBaseSetup) {
        v = baseVerts[mNearestIndex];
      }
      mDragVertexPlane.set(v, mCam.getViewDirection());
      mDragVertexOffset = v - screenToWorldAtVertexPlane(mMousePos,
                                                         mDragVertexPlane);
      mVertexDragging = true;
    }
  }
}


void LandscapeApp::mouseMove( MouseEvent event )
{
  mMousePos = event.getPos();
}


void LandscapeApp::mouseUp( MouseEvent event)
{
  mVertexDragging = false;
}


void LandscapeApp::mouseDrag( MouseEvent event )
{
  mouseMove( event );
  if( mMouseEdit ) {
    mMayaCam.mouseDrag(event.getPos(),
                       mMouseLMB,
                       mMouseMMB,
                       mMouseRMB );
  } else {
    if (mVertexDragging) {
      auto worldPosition = screenToWorldAtVertexPlane(mMousePos,
                                                      mDragVertexPlane);
      if (mSetup) {
        if (mBaseSetup) {
          baseVerts[mNearestIndex] = worldPosition + mDragVertexOffset;
        } else {
          for (auto i : mNearestIndicies) {
            mMesh.getVertices()[i] = worldPosition + mDragVertexOffset;
          }
          mVBO = gl::VboMesh::create( mMesh, mLayout );
        }
      }
    }
  }
}


void LandscapeApp::drawShadowCasters(gl::GlslProg* deferShader) const
{
  gl::pushMatrices();
  gl::draw( mVBO );
  gl::popMatrices();
}


void LandscapeApp::drawNonShadowCasters(gl::GlslProg* deferShader) const
{
  //a plane to capture shadows (though it won't cast any itself)
  glColor3ub(255, 255, 255);
  glNormal3f(0.0f, 1.0f, 0.0f);
  glBegin(GL_QUADS);
  gl::vertex(baseVerts[0]);
  gl::vertex(baseVerts[1]);
  gl::vertex(baseVerts[2]);
  gl::vertex(baseVerts[3]);
  glEnd();
}


void LandscapeApp::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
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
  return { PyFloat_AsDouble(az), PyFloat_AsDouble(alt), DEFAULT_LIGHT_DIST};
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
  return {PyFloat_AsDouble(az), PyFloat_AsDouble(alt), DEFAULT_LIGHT_DIST};
}


CINDER_APP_BASIC( LandscapeApp, RendererGl )
