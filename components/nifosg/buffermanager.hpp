
#ifndef BUFFMANAGER_H
#define BUFFMANAGER_H 1
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/Quat>
#include <osg/Vec3>
#include <osg/MatrixTransform>
#include <osg/Texture2D>
#include <osg/PolygonStipple>
#include <osg/TriangleFunctor>
#include <osg/io_utils>
#include <assert.h>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgUtil/MeshOptimizers>
#include <osgGA/TrackballManipulator>
#include <osgGA/FirstPersonManipulator>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <osg/Math>

#include <iostream>



///2)transform primset to their basevertex equivalents
///3)maximize BOset sharing among SharedVAOGeometries :1VAS/BOset (responsible of its create is so called master geometry )
///if attribute are per vertex nothing can go wrong, all arrays will share basevertex in their bo
///4)compute primsets basevertices according bo structure

namespace NifOsg
{
struct MasterGeomVertexArrayStateCallback : public virtual osg::Drawable::CreateVertexArrayStateCallback
{
    mutable  const osg::Drawable* _master;
    MasterGeomVertexArrayStateCallback(osg::Drawable *dr=0):_master(0) {}

    void setMasterGeometry(osg::Drawable*dr) {
        _master=dr;
    }
    const osg::Drawable * getMasterGeometry()const {
        return _master;
    }
    MasterGeomVertexArrayStateCallback(const MasterGeomVertexArrayStateCallback& rhs,const osg::CopyOp& copyop):
        osg::Drawable:: CreateVertexArrayStateCallback(rhs, copyop) {
        _master=rhs._master;
    }
    //     osg::Drawable::VertexArrayStateCallback(rhs, copyop) {_master=rhs._master;}

    META_Object(osg, MasterGeomVertexArrayStateCallback);

    /** do customized createVertexArrayState .*/
    virtual osg::VertexArrayState* createVertexArrayStateImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* dr ) const ;
};
class  MakeSharedBufferObjectsVisitor : public osg::NodeVisitor
{
public:
    void apply(osg::Geometry&g);
    MakeSharedBufferObjectsVisitor();
    // virtual void apply(osg::Geometry& transform);
    void setHardBufferSize(unsigned int i)    {
        _hardMaxbuffsize=i;
    }
    unsigned int getHardBufferSize()const    {
        return _hardMaxbuffsize;
    }
    void setSoftBufferSize(unsigned int i)    {
        _softMaxbuffsize=i;
    }
    unsigned int getSoftBufferSize()const    {
        return _softMaxbuffsize;
    }
    unsigned int getNumBufferSetGenerated()const    {
        return _numVAOsInUsed;
    }

    bool isHackActivated()const    {
        return _hack;
    }
    void setIsHackActivated(bool i)    {
        _hack=i;
    }

    osg::Geometry*  treatBufferObjects(osg::Geometry* g);
    unsigned int _lastKeyUsed;///for stats
protected:

  //  typedef std::vector< osg::observer_ptr<osg::BufferObject> > BuffSet;
    typedef std::vector< osg::ref_ptr<osg::BufferObject> > BuffSet;

    typedef  std::pair<BuffSet,osg::Geometry *>  BuffSetAndMaster;
    typedef  std::vector< BuffSet/*AndMaster*/ > VecBuffSetAndMaster;

    std::map<unsigned int,VecBuffSetAndMaster > _store;

    unsigned int _hardMaxbuffsize;///prohibit bufferdata concatenation in bufferobject
    unsigned int _softMaxbuffsize;///hint a bufferobject is full (and increment lastempty)
    unsigned int _numVAOsInUsed;///for stats

    bool _hack;
};

class  RemoveVertexArrayUserDataVisitor : public osg::NodeVisitor{
public:
    void apply(osg::Geometry&g){g.getVertexArray()->setUserData(0);}
    RemoveVertexArrayUserDataVisitor():
        osg::NodeVisitor(osg::NodeVisitor::NODE_VISITOR,osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
};
class BufferManager{
public:
static osg::ref_ptr<MakeSharedBufferObjectsVisitor> vaovis;
};
}
#endif
