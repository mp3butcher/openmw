

#include "components/nifosg/buffermanager.hpp"
#include <components/settings/settings.hpp>

#define DONTSETUPBOATUPDATE 1

namespace NifOsg
{
osg::ref_ptr<NifOsg::MakeSharedBufferObjectsVisitor> BufferManager::vaovis=new NifOsg::MakeSharedBufferObjectsVisitor;
/**add drawables from cull or render thread**/
#define UPDATER_CAM
/**BaseVertex Primimitive Sets**/
#define BASEARRAY 1
class DrawArraysBaseVertex : public osg::DrawArrays
{
public:
#ifdef BASEARRAY
    osg::Array*getBaseVertexArray() {
        return _basevertex;
    }
    void setBaseVertexArray(osg::Array*a) {
        _basevertex=a;
    }
    osg::ref_ptr<osg::Array> _basevertex;
#else
    GLint _basevertex;
#endif
    DrawArraysBaseVertex(GLenum mode=0):
        osg::DrawArrays(mode) {}

    DrawArraysBaseVertex(

        const osg::DrawArrays& array,

        const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY):
        osg::DrawArrays(array,copyop)    {

        _mode=array.getMode();
        _first=array.getFirst();
        _count=array.getCount();

    }
    virtual void draw(osg::State& state, bool useVertexBufferObjects) const
    {
#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE)
        GLenum mode = _mode;
        if (_mode==GL_QUADS)
        {
            state.drawQuads(_first+_basevertex, _count, _numInstances);
            return;
        }
        else if (mode==GL_POLYGON)
        {
            mode = GL_TRIANGLE_FAN;
        }
        else if (mode==GL_QUAD_STRIP)
        {
            mode = GL_TRIANGLE_STRIP;
        }

        if (_numInstances>=1) state.glDrawArraysInstanced(mode,_first+_basevertex,_count, _numInstances);
        else glDrawArrays(mode,_first+_basevertex,_count);
#else
#if 0
        const osg::Array * array=pate->getVertexArray();
        if(!array)array=pate->getVertexAttribArray(0);
        array->getBufferObject()->getProfile()._size;
        osg::GLBufferObject* globj=array->getOrCreateGLBufferObject(state.getContextID());
        assert(globj->getOffset(array->getBufferIndex())/                (array->getElementSize())==_basevertex);
        // std::cerr<<pate<<"< "<<  globj->getOffset(array->getBufferIndex())/                (array->getElementSize()) << "== "<<_basevertex<<std::endl;
#endif

#ifdef BASEARRAY
        GLint bv=((unsigned int)_basevertex->getOrCreateGLBufferObject(state.getContextID())->getOffset(_basevertex->getBufferIndex()))/_basevertex->getElementSize();

        if (_numInstances>=1) state.glDrawArraysInstanced(_mode,_first+bv,_count, _numInstances);
        else glDrawArrays(_mode,_first+bv,_count);
#else
        // std::cout<<_first<<" "<<_basevertex<<std::endl;
        if (_numInstances>=1) state.glDrawArraysInstanced(_mode,_first+_basevertex,_count, _numInstances);
        else glDrawArrays(_mode,_first+_basevertex,_count);
#endif
#endif

    }
};

#define concat(first, second) first second
#define XSTR(X) #X
template< class T,unsigned int DrawType>
class DrawElementsBaseVertex : public T /*osg::DrawElementsUInt*/
{
public:
    DrawElementsBaseVertex(GLenum mode=0):
        T(mode) {}

    DrawElementsBaseVertex(const T& array, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY):
        T(array,copyop)
    {    }
    DrawElementsBaseVertex(const DrawElementsBaseVertex& array, const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY):
        T(array,copyop)
    {
        _basevertex=array._basevertex;
    }
    virtual osg::Object* cloneType() const
    {
        return new DrawElementsBaseVertex<T,DrawType>();
    }
    virtual osg::Object* clone(const osg::CopyOp& copyop) const
    {
        return new DrawElementsBaseVertex<T,DrawType>(*this,copyop);
    }
    virtual bool isSameKindAs(const osg::Object* obj) const
    {
        return dynamic_cast<const DrawElementsBaseVertex<T,DrawType>*>(obj)!=NULL;
    }

    virtual const char* libraryName() const
    {
        return "osgMW";
    }
    virtual const char* className() const
    {
        return T::className();//concat("DrawElementsBaseVertex<##T,##DrawType>",XSTR(##DrawType));
    }

    virtual void accept(osg::PrimitiveFunctor& functor) const {

    }
    virtual void accept(osg::PrimitiveIndexFunctor& functor) const {

    }

#ifdef BASEARRAY
    osg::Array*getBaseVertexArray() {
        return _basevertex;
    }
    void setBaseVertexArray(osg::Array*a) {
        _basevertex=a;
    }
#else
    /// NEWVAS : virtual added in DrawElements in order to be passed to array dispatcher at VAS compile time
    virtual GLint getBaseVertex()const {
        return _basevertex;
    }
#endif
    virtual void draw(osg::State& state, bool useVertexBufferObjects) const
    {
        osg:: GLExtensions* ext = state.get<osg::GLExtensions>();

        GLenum mode = T::_mode;
#if defined(OSG_GLES1_AVAILABLE) || defined(OSG_GLES2_AVAILABLE)
        if (mode==GL_POLYGON) mode = GL_TRIANGLE_FAN;
        if (mode==GL_QUAD_STRIP) mode = GL_TRIANGLE_STRIP;
#endif

        if (useVertexBufferObjects)
        {
            osg::GLBufferObject* ebo = T::getOrCreateGLBufferObject(state.getContextID());
#if 0
            const osg::Array * array=pate->getVertexArray();
            if(!array)array=pate->getVertexAttribArray(0);
            array->getBufferObject()->getProfile()._size;
            osg::GLBufferObject* globj=array->getOrCreateGLBufferObject(state.getContextID());
            std::cerr<<pate<<"< "<<  globj->getOffset(array->getBufferIndex())/                (array->getElementSize()) << "== "<<_basevertex<<std::endl;
#endif
            if (ebo)//->getBufferObject())
            {

#ifdef BASEARRAY
                osg::GLBufferObject* vbo= _basevertex->getOrCreateGLBufferObject(state.getContextID());
                if(!vbo)return;
                if(vbo->isDirty())   //                return;
                    vbo->compileBuffer();
                GLint bv=((unsigned int)vbo->getOffset(_basevertex->getBufferIndex()))/_basevertex->getElementSize();
                //          OSG_WARN<<bv<<"< "<< _basevertex->getOrCreateGLBufferObject(state.getContextID())->getOffset(_basevertex->getBufferIndex()) << "== "<<_basevertex->getElementSize()<<std::endl;
                /*        GLint bv2=0;
                   osg::BufferObject * bo=_basevertex->getBufferObject();
                    for(int i=0; i<_basevertex->getBufferIndex(); ++i) {
                        osg::Array * arr=bo->getBufferData(i)->asArray();
                        bv2+=arr->getNumElements();
                    }
                if(bv!=bv2)
                  OSG_WARN<<bv<<"< "<< bv2<<_basevertex->getUserData()<<std::endl; */
                state.bindElementBufferObject(ebo);
                if (T::_numInstances>1) ext->glDrawElementsInstancedBaseVertex(mode, T::size(), DrawType, (const GLvoid *)(ebo->getOffset(T::getBufferIndex())), T::_numInstances
                            ,bv);
                else ext->glDrawElementsBaseVertex(mode, T::size(), DrawType, (const GLvoid *)(ebo->getOffset(T::getBufferIndex())),bv);

#else
                state.bindElementBufferObject(ebo);
                if (T::_numInstances>=1) ext->glDrawElementsInstancedBaseVertex(mode, T::size(), DrawType, (const GLvoid *)(ebo->getOffset(T::getBufferIndex())), T::_numInstances
                            ,_basevertex);
                else ext->glDrawElementsBaseVertex(mode, T::size(), DrawType, (const GLvoid *)(ebo->getOffset(T::getBufferIndex()))
                                                       ,_basevertex
                                                      );
#endif
            }
            /*else
            {
                state. unbindElementBufferObject();
                if (T::_numInstances>=1) state.glDrawElementsInstanced(mode, T::size(), DrawType, &T::front(), T::_numInstances);
                else glDrawElements(mode, T::size(), DrawType, &T::front());
            }*/
        }
        else
        {
            if (T::_numInstances>=1) state.glDrawElementsInstanced(mode, T::size(), DrawType, &T::front(), T::_numInstances);
            else glDrawElements(mode, T::size(), DrawType, &T::front());
        }
    }

#ifdef BASEARRAY
    // osg::ref_ptr<osg::Array>
    osg::Array*
#else
    GLint
#endif
    _basevertex;
};
typedef DrawElementsBaseVertex<osg::DrawElementsUInt,GL_UNSIGNED_INT> DrawElementsBaseVertexUINT;
typedef DrawElementsBaseVertex<osg::DrawElementsUShort,GL_UNSIGNED_SHORT> DrawElementsBaseVertexUSHORT;
typedef DrawElementsBaseVertex<osg::DrawElementsUByte,GL_UNSIGNED_BYTE> DrawElementsBaseVertexUBYTE;


void setPrimitivesBaseVertex(osg::Geometry *g) {
    g->getVertexArray()->setUserData(g);
    if(g->getVertexArray()) {
#ifdef BASEARRAY
        g->getVertexArray()->setUserData(g);
        osg::Array *basevertex=g->getVertexArray();
        osg::Geometry::ArrayList arrays;
        osg::Geometry::DrawElementsList drs;
        g->getArrayList(arrays);
        g->getDrawElementsList(drs);
        GLuint gbasevertex=0;
        /**/  for(auto   arr:arrays)
        {
            GLuint basevertex=0;
            for(int i=0; i<arr->getBufferObject()->getNumBufferData(); i++) {
                ///  basevertex+=((osg::Array*)arr->getBufferObject()->getBufferData(i))->getNumElements();

                arr->getBufferObject()-> getBufferData(i)->dirty();
                if(arrays[0]==arr)if(arrays[0]->getBufferObject()->getBufferData(i)->getUserData()) {
                    osg::ref_ptr<osg::Geometry>geom2=   static_cast<osg::Geometry*>(arrays[0]->getBufferObject()->getBufferData(i)->getUserData());
                    geom2  ->osg::Drawable::releaseGLObjects();
                    osg::Geometry::ArrayList arrays2;
                    osg::Geometry::DrawElementsList drs2;
                    geom2->getArrayList(arrays2);
                    geom2->getDrawElementsList(drs2);
                    for(auto   arr2:arrays2)arr2->dirty();
                    for(auto   arr2:drs2)arr2->dirty();
                }
                osg::Geometry::ArrayList arrays;
                osg::Geometry::DrawElementsList drs;
                g->getArrayList(arrays);
                g->getDrawElementsList(drs);
            }
            arr->getBufferObject()-> dirty();
            arr->dirty();
            arr->getBufferObject()-> getBufferData(0)->dirty();
            ///   if(basevertex!=gbasevertex &&gbasevertex!=0)OSG_WARN<<"fok"<<basevertex<<std::endl;
            /// gbasevertex=basevertex;

        }
        for(auto   arr:drs)
        {
            arr->getBufferObject()-> dirty();
            arr->dirty();
            for(int i=0; i<arr->getBufferObject()->getNumBufferData(); i++) {
                arr->getBufferObject()-> getBufferData(i)->dirty();
            }

        }
        g->dirtyBound();
        // g ->setCullingActive(false);//force master draw
        osg::BufferObject * bo = arrays[0]->getBufferObject();
        for(int i=0; i<bo->getNumBufferData(); i++) {
            //   ((osg::Drawable*)  bo->getBufferData(i)->getUserData()) ->setCullingActive(false);
          if( bo->getBufferData(i)->getUserData())  ((osg::Drawable*)  bo->getBufferData(i)->getUserData())  -> osg::Drawable::releaseGLObjects();//destroy shared master vas
            //   ((osg::Drawable*)  bo->getBufferData(i)->getUserData())  ->osg::Drawable::dirtyGLObjects();//destroy shared master vas
        }
  //      ((osg::Drawable*)  arrays[0]->getBufferObject()->getBufferData(0)->getUserData())  ->setCullingActive(false);//force master draw
    //    ((osg::Drawable*)  arrays[0]->getBufferObject()->getBufferData(0)->getUserData())  -> osg::Drawable::releaseGLObjects();//destroy shared master vas
        //
#define BASEAFFECT(W) setBaseVertexArray(W)
#else
        GLint basevertex=0;
        uint elmtsize=static_cast<osg::Array*>(g->getVertexArray()->getBufferObject()->getBufferData(0))->getElementSize();

        for(int i=0; i<g->getVertexArray()->getBufferIndex(); i++) {
            assert(static_cast<osg::Array*>(g->getVertexArray()->getBufferObject()->getBufferData(i))->getElementSize()==elmtsize);
            basevertex+=g->getVertexArray()->getBufferObject()->getBufferData(i)->getTotalDataSize();
        }
        assert(g->getVertexArray()->getElementSize()==elmtsize);
        basevertex/=g->getVertexArray()->getElementSize();
///  setbasevertex without the pointer hack in drawElementsbasevertex
///
#define BASEAFFECT(W) _basevertex=(W)
#endif

        osg::Geometry::PrimitiveSetList& drawelmts=g->getPrimitiveSetList();

        for(osg::Geometry::PrimitiveSetList::iterator prit=drawelmts.begin(); prit!=drawelmts.end(); prit++)
        {
            if(dynamic_cast<osg::DrawElementsUByte*>(prit->get()))
                dynamic_cast<DrawElementsBaseVertexUBYTE*>(prit->get())->BASEAFFECT(basevertex);
            else if(dynamic_cast<osg::DrawElementsUInt*>(prit->get()))
                dynamic_cast<DrawElementsBaseVertexUINT*>(prit->get())->BASEAFFECT(basevertex);
            else if(dynamic_cast<osg::DrawElementsUShort*>(prit->get()))
                dynamic_cast<DrawElementsBaseVertexUSHORT*>(prit->get())->BASEAFFECT(basevertex);
            else    if(dynamic_cast<osg::DrawArrays*>(prit->get())) {
                //computebound fail with classic DrawArray   dynamic_cast<DrawArraysBaseVertex*>(prit->get())->setFirst( basevertex);//basevertex;
                //don't understand why drawablecomputebound is based on primitivefunctor and not simply on the parsing the whole vertexarray ...kinda overkill
                dynamic_cast<DrawArraysBaseVertex*>(prit->get())->BASEAFFECT(basevertex);//basevertex;
            }
        }
    }
}

///return Hash without lastbit (index array bit)
#define MAX_TEX_COORD 8
#define MAX_VERTEX_ATTRIB 16
///in case of array binding != PER_VERTEX drop the array
#define HACKARRAY(GEOM,ARRAYPROP) if (GEOM->get##ARRAYPROP() && GEOM->get##ARRAYPROP()->getBinding()!=osg::Array::BIND_PER_VERTEX) {OSG_DEBUG<<#ARRAYPROP<<"removed"<<std::endl; GEOM->set##ARRAYPROP(0);}
#define HACKARRAYS(GEOM,ARRAYPROP,I) if (GEOM->get##ARRAYPROP(I) && GEOM->get##ARRAYPROP(I)->getBinding()!=osg::Array::BIND_PER_VERTEX){OSG_DEBUG<<#ARRAYPROP<<"removed"<<std::endl;GEOM->set##ARRAYPROP(I,0);}

unsigned int getArrayList(osg::Geometry*g,osg::Geometry::ArrayList &arrayList)
{
    unsigned int hash=0;
    HACKARRAY(g,VertexArray)
    HACKARRAY(g,NormalArray)
    HACKARRAY(g,ColorArray)
    HACKARRAY(g,SecondaryColorArray)
    HACKARRAY(g,FogCoordArray)
   // if(!g->getNormalArray())g->setNormalArray(new osg::Vec3Array(g->getVertexArray()->getNumElements()),osg::Array::BIND_PER_VERTEX);
   // if(!g->getColorArray())g->setColorArray(new osg::Vec4Array(g->getVertexArray()->getNumElements()),osg::Array::BIND_PER_VERTEX);
   // if(!g->getTexCoordArray(0))g->setTexCoordArray(0,new osg::Vec2Array(g->getVertexArray()->getNumElements()),osg::Array::BIND_PER_VERTEX);

    if (g->getVertexArray())
    {
        hash++;
        arrayList.push_back(g->getVertexArray());
    }
    hash<<=1;
    if (g->getNormalArray())
    {
        hash++;
        arrayList.push_back(g->getNormalArray());
    }
    hash<<=1;
     if (g->getColorArray())
    {
        hash++;
        arrayList.push_back(g->getColorArray());
    }
    hash<<=1;
    if (g->getSecondaryColorArray())
    {
        hash++;
        arrayList.push_back(g->getSecondaryColorArray());
    }
    hash<<=1;
    if (g->getFogCoordArray())
    {
        hash++;
        arrayList.push_back(g->getFogCoordArray());
    }
    hash<<=1;
    for(unsigned int unit=0; unit<g->getNumTexCoordArrays(); ++unit)
    {
        HACKARRAYS(g,TexCoordArray,unit)
        osg::Array* array = g->getTexCoordArray(unit);
        if (array)
        {
            hash++;
            arrayList.push_back(array);
        }
        hash<<=1;
    }
    hash<<=MAX_TEX_COORD-g->getNumTexCoordArrays();
    for(unsigned int  index = 0; index <g->getNumVertexAttribArrays(); ++index )
    {
        HACKARRAYS(g,VertexAttribArray,index)
        osg::Array* array =g->getVertexAttribArray(index);
        if (array)
        {
            hash++;
            arrayList.push_back(array);
        }
        hash<<=1;
    }
    hash<<=MAX_VERTEX_ATTRIB-g->getNumVertexAttribArrays();
    return hash;
}


///check bufferobject already guesting the bd
bool isGuesting(const osg::BufferObject&bo,const osg::BufferData*bd)
{
    for(unsigned int i=0; i<bo.getNumBufferData(); i++)
        if(bo.getBufferData(i)==bd)return true;
    return false;
}

/** Shared VAS createVASCallback **/
class HackGeom :public osg::Geometry {
public:
    osg::VertexArrayState*createVAS(osg::RenderInfo &s) {
        //  osg::VertexArrayState*createVAS(osg::State*s) {
        return createVertexArrayStateImplementation(s);
    }
};

osg::VertexArrayState* MasterGeomVertexArrayStateCallback::createVertexArrayStateImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* dr ) const {
        osg::State*s=renderInfo.getState();

        //   virtual osg::VertexArrayState* operator ()(osg::State* s, const osg::Drawable* dr/*drawable*/)const {

#ifdef  DONTSETUPBOATUPDATE
       // if(!dr->asGeometry()->getVertexArray()->getUserData())
   //   if(!dr->asGeometry()->getVertexArray()->getBufferObject())
     BufferManager::vaovis->treatBufferObjects(const_cast< osg::Geometry*>(dr->asGeometry()));
    if( BufferManager::vaovis->_lastKeyUsed!=0)
    {
const osg::Group *node;
        if(dr->getNumParents()>0)node=dr->getParent(0);else node=0;
osg::StateSet* ss=const_cast<osg::StateSet*>(dr->getStateSet());
while(!ss&&node)
{
    ss=const_cast<osg::StateSet*>(node->getStateSet());
    if(node->getNumParents()>0)node=node->getParent(0);else node=0;
}
if(ss)
ss->setRenderBinDetails( BufferManager::vaovis->_lastKeyUsed, "RenderBin");
    }

#endif
      /** do customized createVertexArrayState .*/
          osg::ref_ptr<const osg::BufferData> firstbd;
          //  if(!((const osg::Geometry*)dr)->getVertexArray()->getBufferObject())return dr->createVertexArrayStateImplementation(renderInfo);
          for(int i=0; i<((const osg::Geometry*)dr)->getVertexArray()->getBufferObject()->getNumBufferData(); ++i) {
              firstbd=((const osg::Geometry*)dr)->getVertexArray()->getBufferObject()->getBufferData(i);
              /*   if(!firstbd->getGLBufferObject(renderInfo.getContextID()))
                     dynamic_cast<const osg::Geometry*>(firstbd->getUserData())->compileGLObjects(renderInfo);*/
              break;
          }
          /// set Geometry owning bd as userdata

          osg::ref_ptr<const osg::Geometry > mastergeom=dynamic_cast<const osg::Geometry*>(firstbd->getUserData());
          OSG_WARN<<"MasterGeomVertexArrayStateCallback"<<_master <<" "<<mastergeom<<std::endl;
          osg::ref_ptr< osg::Geometry > hackedgeom;
          osg::Geometry * masterg=const_cast< osg::Geometry*>(mastergeom.get()); //(firstbd->getUserData());
          // if(!mastergeom)return dr->createVertexArrayStateImplementation(renderInfo);
          //
          //if(mastergeom)
          osg::ref_ptr<osg::VertexArrayState >  vas=mastergeom->getVertexArrayStateList()[s->getContextID()];
          if(mastergeom==dr)
              const_cast< osg::Drawable*>(dr)->setCullingActive(false);
          else const_cast< osg::Drawable*>(dr)->setCullingActive(true);
          if(vas&&mastergeom!=dr) { //&&mastergeom==_master)  {
              //  OSG_WARN<<"reuse vas"<<std::endl;
              _master=mastergeom;
              const_cast< osg::Drawable*>(dr)->getVertexArrayStateList()[s->getContextID()]=vas;
              mastergeom->draw(renderInfo);
              //mastergeom->dirtyGLObjects();
              // dr->compileGLObjects(renderInfo);
              if(!vas)
              {
                  OSG_WARN<<"mzster drawincorrect vas"<<_master <<" "<<mastergeom<<std::endl;
                  vas=((HackGeom*) mastergeom.get())->createVAS(renderInfo);
                  mastergeom->draw(renderInfo);
                  vas=    const_cast<osg::Geometry*>( mastergeom.get())->getVertexArrayStateList()[s->getContextID()];
              }
#define RET              s->setCurrentVertexArrayState(0);  return vas;
              RET
          }

          // if(vas->get)
          /*if(_master){_master->Drawable::releaseGLObjects(renderInfo.getState());
                OSG_WARN<<"release vas"<<std::endl;
          }*/
          //  mastergeom->releaseGLObjects(renderInfo.getState());

          //  OSG_WARN<<"new vas"<<_master <<" "<<mastergeom<<std::endl;
          // static_cast<const osg::Drawable*>(
          _master=mastergeom;
          if(mastergeom!=dr)
          {
              //vas=const_cast<osg::Geometry*>( mastergeom.get())->getVertexArrayStateList()[s->getContextID()];
              //if(!vas)
              const_cast<osg::Geometry*>( mastergeom.get())->getVertexArrayStateList()[s->getContextID()]=vas=((HackGeom*) mastergeom.get())->createVAS(renderInfo);

              mastergeom->draw(renderInfo);
              vas=    const_cast<osg::Geometry*>( mastergeom.get())->getVertexArrayStateList()[s->getContextID()];
              if(!vas)
              {
                  OSG_WARN<<"mzster drawincorrect vas"<<_master <<" "<<mastergeom<<std::endl;
                  vas=((HackGeom*) mastergeom.get())->createVAS(renderInfo);
                  mastergeom->draw(renderInfo);
                  vas=    const_cast<osg::Geometry*>( mastergeom.get())->getVertexArrayStateList()[s->getContextID()];
              }
              //mastergeom->compileGLObjects(renderInfo);
              // vas=mastergeom->getVertexArrayStateList()[s->getContextID()];
              /* else for(int i=0; i<((const osg::Geometry*)dr)->getVertexArray()->getBufferObject()->getNumBufferData(); ++i) {

                       firstbd=((const osg::Geometry*)dr)->getVertexArray()->getBufferObject()->getBufferData(i);
                       hackedgeom=  ((osg::Geometry*)firstbd->getUserData());
                       //  ((osg::Geometry*)firstbd->getUserData())->osg::Drawable::dirtyGLObjects();
                       // ((osg::Geometry*)firstbd->getUserData())->getVertexArrayStateList().resize(s->getContextID()+1);
                       if(hackedgeom.valid())                    hackedgeom->getVertexArrayStateList()[s->getContextID()]=vas ;
                   }*/
             RET
          }
          else
          {
              // (mastergeom==dr)
              /*   for(int i=0; i<((const osg::Geometry*)dr)->getVertexArray()->getBufferObject()->getNumBufferData(); ++i) {
                     firstbd=((const osg::Geometry*)dr)->getVertexArray()->getBufferObject()->getBufferData(i);
                     if(false)
                         ((osg::Geometry*)firstbd->getUserData())->osg::Drawable::dirtyGLObjects();
                 }
                 for(int i=0; i<((const osg::Geometry*)dr)->getVertexArray()->getBufferObject()->getNumBufferData(); ++i) {

                     firstbd=((const osg::Geometry*)dr)->getVertexArray()->getBufferObject()->getBufferData(i);

                     // ((osg::Geometry*)firstbd->getUserData())->osg::Drawable::dirtyGLObjects();
                     // ((osg::Geometry*)firstbd->getUserData())->getVertexArrayStateList().resize(s->getContextID()+1);
                     // ((osg::Geometry*)firstbd->getUserData())->getVertexArrayStateList()[s->getContextID()]=0 ;
                 }*/
              vas=((HackGeom*) mastergeom.get())->createVAS(renderInfo);
              const_cast<osg::Geometry*>( mastergeom.get())->getVertexArrayStateList()[s->getContextID()]=vas.get();
              //   mastergeom->compileGLObjects(renderInfo);
              mastergeom->draw(renderInfo);

              vas=    const_cast<osg::Geometry*>( mastergeom.get())->getVertexArrayStateList()[s->getContextID()];
              if(!vas)
              {
                  OSG_WARN<<"mzster drawincorrect vas"<<_master <<" "<<mastergeom<<std::endl;
                  vas=((HackGeom*) mastergeom.get())->createVAS(renderInfo);
              }
              /*   for(int i=0; i<((const osg::Geometry*)dr)->getVertexArray()->getBufferObject()->getNumBufferData(); ++i) {

                  firstbd=((const osg::Geometry*)dr)->getVertexArray()->getBufferObject()->getBufferData(i);
                  hackedgeom=  ((osg::Geometry*)firstbd->getUserData());
                  //  ((osg::Geometry*)firstbd->getUserData())->osg::Drawable::dirtyGLObjects();
                  // ((osg::Geometry*)firstbd->getUserData())->getVertexArrayStateList().resize(s->getContextID()+1);
                  if(hackedgeom.valid())                    hackedgeom->getVertexArrayStateList()[s->getContextID()]=vas ;
              }*/
              // vas=mastergeom->getVertexArrayStateList()[s->getContextID()];
  //mastergeom->compileGLObjects(renderInfo);


          }
  #if 0
          if(mastergeom==dr) {
              osg::State::SetCurrentVertexArrayStateProxy setVASProxy(*renderInfo.getState(), vas);
              renderInfo.getState()->bindVertexArrayObject(vas);
              mastergeom->drawVertexArraysImplementation(renderInfo);
              renderInfo.getState()->unbindVertexArrayObject();
          }
  #endif

        if(!vas)
            OSG_WARN<<"VAO null:WTF"<<std::endl;
       RET

    }

osg::Geometry*  MakeSharedBufferObjectsVisitor::treatBufferObjects(osg::Geometry* g)
{
_lastKeyUsed=0;
    osg::Geometry::ArrayList  bdlist;
    unsigned int hash= getArrayList(g,bdlist),hasht;


    osg::Geometry::PrimitiveSetList newdrawelmts;

    ///convert primset to BaseVertex
#if 1
    osg::Geometry::PrimitiveSetList& prlist=g->getPrimitiveSetList();;
    for(osg::Geometry::PrimitiveSetList::iterator prit=prlist.begin(); prit!=prlist.end(); prit++)
    {
        if(std::string(prit->get()->libraryName())=="osgMW") return 0;
        if(dynamic_cast<osg::DrawElementsUByte*>(prit->get()))
            newdrawelmts.push_back( new DrawElementsBaseVertexUBYTE(*dynamic_cast<osg::DrawElementsUByte*>(prit->get()),osg::CopyOp(osg::CopyOp::DEEP_COPY_ALL)));
        else if(dynamic_cast<osg::DrawElementsUInt*>(prit->get()))
            newdrawelmts.push_back( new DrawElementsBaseVertexUINT(*dynamic_cast<osg::DrawElementsUInt*>(prit->get()),osg::CopyOp(osg::CopyOp::DEEP_COPY_ALL)));
        else if(dynamic_cast<osg::DrawElementsUShort*>(prit->get()))
            newdrawelmts.push_back( new DrawElementsBaseVertexUSHORT(*dynamic_cast<osg::DrawElementsUShort*>(prit->get()),osg::CopyOp(osg::CopyOp::DEEP_COPY_ALL)));
        else if(dynamic_cast<osg::DrawArrays*>(prit->get()))            newdrawelmts.push_back( new DrawArraysBaseVertex(*dynamic_cast<osg::DrawArrays*>(prit->get()), osg::CopyOp(osg::CopyOp::DEEP_COPY_ALL)));
        else
        {
            std::cout<<"Not taken into account"<<std::endl;
            return 0;
        }
        // g->removePrimitiveSet(g->getPrimitiveSetIndex(*prit));

    }
    OSG_WARN<<"treating "<<g->className()<<" "<<g->getName()<<std::endl;
    _lastKeyUsed=hash;
    ///basevertices will be setted later
    g->removePrimitiveSet(0,g->getNumPrimitiveSets());
    for(osg::Geometry::PrimitiveSetList::iterator prit=newdrawelmts.begin(); prit!=newdrawelmts.end(); prit++)
        g->addPrimitiveSet(*prit);
#endif

    osg::Geometry::DrawElementsList drawelmts;
    //std::cout<<bdlist.size()<<" "<<hash<<std::endl;
    drawelmts.clear();
    g->getDrawElementsList(drawelmts);
    hasht=hash;
    unsigned int nbborequired=bdlist.size(),nbdrawelmt=0;


    for(unsigned index=0; index< drawelmts.size(); index++)
    {
        if(nbborequired==bdlist.size())
        {
            nbborequired=bdlist.size()+1;
            hash=hasht+1;
        }
        nbdrawelmt++;
    }

    osg::Geometry::ArrayList::iterator arit;
    BuffSet::iterator itbo;
    VecBuffSetAndMaster::iterator itbuffset;

    VecBuffSetAndMaster & vecBuffSetANDmaster=_store[hash];
#if 1
//check buffers configuration
    if (!vecBuffSetANDmaster.empty()){
        itbo=(*vecBuffSetANDmaster.begin()).begin();
        for(arit=bdlist.begin(); arit!=bdlist.end(); itbo++,arit++ ){
            if((*arit)->getDataSize() != (*itbo)->getBufferData(0)->asArray()->getDataSize())
                OSG_FATAL<<"check buffers configuration  "<<(*arit)->getDataSize()<< " != "<<(*itbo)->getBufferData(0)->asArray()->getDataSize()<<std::endl;
        }
    }
    if( !drawelmts.empty() ){//itbo!=(*vecBuffSetANDmaster.begin()).end()){
        //theres an index

    }

#endif

    itbuffset=  vecBuffSetANDmaster.begin();//+vecBuffSet.lastempty;

    unsigned int * buffSize=new unsigned int [nbborequired+nbdrawelmt],*ptrbuffsize=buffSize;

    //while !itbuffset.canGuest(bdlist)
    bool canGuest=false;
    bool alreadyGuesting=true;
    while(!canGuest && itbuffset !=vecBuffSetANDmaster.end())
    {

        itbo=(*itbuffset).begin();

        canGuest=!(*itbuffset).empty() &&(*itbo).get();
        alreadyGuesting=!(*itbuffset).empty()&&(*itbo).get();

        for(arit=bdlist.begin(); arit!=bdlist.end(); itbo++,arit++,ptrbuffsize++)
        {
            *ptrbuffsize=(*itbo)->computeRequiredBufferSize()+(*arit)->getTotalDataSize();
            if(*ptrbuffsize>_hardMaxbuffsize)
                canGuest=false;
            ///check bufferobject already guesting the bd
            if(!isGuesting(**itbo,(*arit)))alreadyGuesting=false;

        }

        if(!drawelmts.empty())
            *ptrbuffsize=(*itbo)->computeRequiredBufferSize();
        for(unsigned index=0; index< drawelmts.size(); index++,ptrbuffsize++)
        {
            *ptrbuffsize+=drawelmts[index]->getTotalDataSize();
            if(*ptrbuffsize>_hardMaxbuffsize)
                canGuest=false;
            ///check bufferobject already guesting the bd
            if(!isGuesting(**itbo,drawelmts[index]))alreadyGuesting=false;
        }

        ptrbuffsize=buffSize; //reset ptr to bufferSizes table
        //DEBUG
        alreadyGuesting=false;
        if(alreadyGuesting)
        {
            delete [] buffSize;

            if(g)
                std::cout<<"alreadyguestl258 reusing vao of "<<g<<std::endl;


            setPrimitivesBaseVertex(g);

#ifndef  DONTSETUPBOATUPDATE
            g->setCreateVertexArrayStateCallback(new  MasterGeomVertexArrayStateCallback(g));
#endif
            //g->setVertexArrayState((*itbuffset).second->getVertexArrayState());
            //  g->setCreateVertexArrayStateCallback((*itbuffset).second->getCreateVertexArrayStateCallback());

            OSG_WARN<<"already guested by the buffer set"<<std::endl;
            return g;
        }
        if(!canGuest)
            itbuffset=vecBuffSetANDmaster.erase(itbuffset);//    itbuffset++;


    }

    //if(itbuffset!=vecBuffSetANDmaster.first.end())//==
    if(canGuest)
    {

        unsigned buffSetMaxSize=0;

        BuffSet::iterator itbo= (*itbuffset).begin();
        //   for(auto bo:itbuffset)bo->lock();
        ptrbuffsize=buffSize;
        for( arit=bdlist.begin(); arit!=bdlist.end(); itbo++,arit++,ptrbuffsize++)
        {
            if(!(*itbo).get())
                OSG_WARN<<"bo fucked"<<std::endl;
            (*arit)->setBufferObject((*itbo).get());
            buffSetMaxSize=buffSetMaxSize<*ptrbuffsize?*ptrbuffsize:buffSetMaxSize;
        }

        for(unsigned index=0; index< drawelmts.size(); index++)
        {   //assert(*itbo == itbuffset->back());
            drawelmts[index]->setBufferObject((*itbo).get());
            buffSetMaxSize=buffSetMaxSize<*ptrbuffsize?*ptrbuffsize:buffSetMaxSize;
            ptrbuffsize++;
        }
        ///check if bufferobject is full against soft limit
        //    if( buffSetMaxSize>_softMaxbuffsize)            vecBuffSetANDmaster.first.erase(itbuffset);

        delete [] buffSize;
        if(!g)
            std::cout<<"l589 reusing vao of "<<g<<std::endl;

        setPrimitivesBaseVertex(g);
        // g->setVertexArrayState((*itbuffset).second->getVertexArrayState());
        // (*itbuffset).second->osg::Drawable::dirtyGLObjects();
#ifndef  DONTSETUPBOATUPDATE
            g->setCreateVertexArrayStateCallback(new  MasterGeomVertexArrayStateCallback(g));
#endif

        return g;
    }



    ///new BuffSetis required
    BuffSet/*AndMaster*/     newBuffSet;
    // newBuffSet.second=g;
    /// Check if buffer object set offsets configuration is compatible with base vertex draw
    int commonoffset=0;
    bool canbereused=true;
    bool ready2go=
#if 1
        false;
#else
        true;
    arit=bdlist.begin();
    if(!bdlist.empty())
    {
        for(uint i=0; i<(*arit)->getBufferIndex(); i++)commonoffset+=(*arit)->getBufferObject()->getBufferData(i)->getTotalDataSize();
        newBuffSet.push_back((*arit)->getBufferObject());
        for( arit++; arit!=bdlist.end()&&ready2go; arit++)
        {
            int offset=0;
            for(uint i=0; i<(*arit)->getBufferIndex(); i++)offset+=(*arit)->getBufferObject()->getBufferData(i)->getTotalDataSize();
            if(offset!=commonoffset)ready2go=false;
            if((*arit)->getBufferObject()->computeRequiredBufferSize()>_softMaxbuffsize)canbereused=false;
            newBuffSet.push_back((*arit)->getBufferObject());
        }
    }
    else
        for(unsigned index=0; index< drawelmts.size()&&ready2go; index++)
        {
            if(drawelmts[0]->getBufferObject()!=drawelmts[index]->getBufferObject())ready2go=false;
            if(drawelmts[index]->getBufferObject()->computeRequiredBufferSize()>_softMaxbuffsize)canbereused=false;
            newBuffSet.push_back(drawelmts[index]->getBufferObject());
            break;
        }
#endif
    ///if configuration is good assume bo set is ready to be used as it is
    if(ready2go)
    {
        if(canbereused) vecBuffSetANDmaster.push_back(newBuffSet);
        delete [] buffSize;
        if(!g)
            std::cout<<"l327 reusing vao of "<<g<<std::endl;

        setPrimitivesBaseVertex(g);
        // g->setVertexArrayState(newBuffSet.second->getVertexArrayState());
#ifndef  DONTSETUPBOATUPDATE
            g->setCreateVertexArrayStateCallback(new  MasterGeomVertexArrayStateCallback(g));
#endif

        return g;
    }
    _numVAOsInUsed++;
    //if(vecBuffSetANDmaster.first.empty())
//   vecBuffSetANDmaster.second=g;

    OSG_WARN<<"create new vao buffset, num vao currently in used:"<<hasht<<" "<<_numVAOsInUsed<<" geom:"<<g<<std::endl;
    ///current configuration doesn't fit so create a new buffset
    newBuffSet.clear();

    osg::ElementBufferObject *ebo=0;
    for( arit=bdlist.begin(); arit!=bdlist.end(); arit++)
    {
        (*arit)->setBufferObject( new osg::VertexBufferObject());
        newBuffSet.push_back( (*arit)->getBufferObject());

    }
    std::cout<<newBuffSet.size()<<" "<<hash<<std::endl;
    if(!drawelmts.empty()) ebo=new osg::ElementBufferObject();
    for(unsigned index=0; index<drawelmts.size(); index++)
    {
        newBuffSet.push_back(ebo);
        drawelmts[index]->setBufferObject(ebo);
    }

    vecBuffSetANDmaster.push_back(newBuffSet);
    delete [] buffSize;
    setPrimitivesBaseVertex(g);
    // g->setVertexArrayState(newBuffSet.second->getVertexArrayState());

    //newBuffSet.second->setCreateVertexArrayStateCallback(new MasterGeomVertexArrayStateCallback(newBuffSet.second));
#ifndef  DONTSETUPBOATUPDATE
            g->setCreateVertexArrayStateCallback(new  MasterGeomVertexArrayStateCallback(g));
#endif
    return g;
}
MakeSharedBufferObjectsVisitor::MakeSharedBufferObjectsVisitor():
    osg::NodeVisitor(osg::NodeVisitor::NODE_VISITOR,osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
{
    _hardMaxbuffsize=1000000000;///min of all glbuffermaxsize
    _softMaxbuffsize=900000000;///ofline: keep all boset during traversal
    _numVAOsInUsed=0;
    _hack=true;
}
void MakeSharedBufferObjectsVisitor::apply(osg::Geometry&g) {
    if(!_hack)return;
    ///force data variance to static
    if(g.getDataVariance()!=osg::Object::STATIC) return;
    if(g.getVertexArray()->getUserData() )return ;
    if(strncmp(g.className(),"Geometry" ,9))
    {
        OSG_WARN<<"MakeSharedBufferObjectsVisitor filtering "<<g.className()<<std::endl;
        return ;
    }
    if(g.getDrawCallback())return ;
    if(g.getUpdateCallback())return ;
    if(g.containsSharedArrays())return ;
   if(! g.isCullingActive())return ;
    g.setUseDisplayList(false);
    g.setUseVertexBufferObjects(true);
    g.setUseVertexArrayObject(true);
    g.setInitialBound(g.getBoundingBox());

    osg::Geometry::ArrayList  bdlist;

    int hashid=getArrayList(&g,bdlist);
    g.getArrayList(bdlist);
    for(osg::Geometry::ArrayList::iterator prit=bdlist.begin(); prit!=bdlist.end(); prit++) {

        if((*prit)->getBinding()!=    osg::Array::BIND_PER_VERTEX) {
            ///cant do anything
            std::cerr<<"throwing geom"<<std::endl;
            return ;
        }
    }
    if(  Settings::Manager::getBool("BOVASrenderbin", "General"))
         g.getOrCreateStateSet()->setRenderBinDetails(hashid,"RenderBin");
    g.getVertexArray()->setUserData(&g);
#ifndef  DONTSETUPBOATUPDATE
    treatBufferObjects(&g);
#else
    //callback call treatBOs
//    g.getVertexArray()->setBufferObject(0);
 //   g.setCullingActive(false);
    g.setCreateVertexArrayStateCallback(new MasterGeomVertexArrayStateCallback(&g));
#endif
}
class fok :public osg::Geometry::ComputeBoundingBoxCallback {
public:
    osg:: BoundingBox _bounds;
    fok( osg::BoundingBox bound):_bounds(bound) {}
    virtual osg:: BoundingBox computeBound(const osg::Drawable&) const  {
        return _bounds;
    }
};

//remove all geom that haven't got any userdata on its first array
class GeomLoaderRemoveCallback : public osg::NodeCallback
{

    osg::NodeList nodelist;
    OpenThreads::Mutex _mutex;
public:
    void addToNodeList(osg::Node* node) {
        OpenThreads::ScopedLock<OpenThreads::Mutex> sm(_mutex);
        nodelist.push_back(node);
    }
    virtual void operator()( osg::Node* node,  osg::NodeVisitor* nv)
    {
#if 1
        osg::Group*  gr  = node->asGroup();
        if(gr->getNumChildren()>0)
        {
            int curch=gr->getNumChildren()-1;
            osg::ref_ptr <osg::Geometry >g=gr->getChild(curch)->asGeometry();
            //while(!g->getCreateVertexArrayStateCallback()&&curch<gr ->getNumChildren()-1)g=gr->getChild(++curch)->asGeometry();

            //if(!g->getVertexArray()->getUserData())
            {   //
                OpenThreads::ScopedLock<OpenThreads::Mutex> sm(_mutex);
                for( auto node= nodelist.begin(); node != nodelist.end(); ) {
                    gr ->removeChild(*node);
                    (*node)->asGeometry()->getVertexArray()->setUserData(0);
                    OSG_WARN<<"removal from SG"<<*node<<std::endl;
                    node= nodelist.erase(node);
                }

            }
        }
#endif
        if(getNestedCallback())
            getNestedCallback()->run(node,nv);
        //(*(osg::NodeCallback*)getNestedCallback())()(node,nv);
    }
};
//add all geoms in nodelist
class GeomLoaderAdderCallback : public osg::NodeCallback {

    osg::NodeList nodelist;
    OpenThreads::Mutex _mutex;
public:
    void addToNodeList(osg::Node* node) {
        OpenThreads::ScopedLock<OpenThreads::Mutex> sm(_mutex);
        nodelist.push_back(node);
    }
    virtual void operator()( osg::Node* node,  osg::NodeVisitor* nv)
    {
#if 1
        {
            osg::Group*  gr  = node->asGroup();
            OpenThreads::ScopedLock<OpenThreads::Mutex> sm(_mutex);
            for( auto node= nodelist.begin(); node != nodelist.end(); ) {
                gr->addChild(*node);
                node= nodelist.erase(node);
            }
        }

        if(getNestedCallback())
            getNestedCallback()->run(node,nv);
#endif
    }
};
class DummyDraw:public osg::Drawable::DrawCallback {
    virtual void drawImplementation(osg::RenderInfo&,const osg::Drawable*)const {};
};
static MakeSharedBufferObjectsVisitor vaovis;
class GeomLoaderCB : public
#ifdef UPDATER_CAM
    osg::Camera::DrawCallback
#else
    osg::NodeCallback
#endif
//
{
public:
    int _thresremoval;
    int _nbaddedatatime;
    GeomLoaderCB(int thresremoval=1,int nbaddedatatime=1):_nbaddedatatime(nbaddedatatime),_thresremoval(thresremoval) {}
    bool _isgeompushfront;
    mutable  std::list<osg::ref_ptr<osg::Geometry> > _geoms;
    int lasttraversal;
    //osgUtil::GeometryCollector::GeometryList _geoms;
    void setGeometryList(osgUtil::GeometryCollector::GeometryList c) {
        for(auto f : c)
            _geoms.push_back(f);
    }
#ifdef UPDATER_CAM
    virtual void operator () (const osg::Camera&  camera ) const {
        osg::Node * node=const_cast<osg::Camera*>(&camera)->getChild(0);
        if(camera.getGraphicsContext()->getState()->getContextID()!=0)return;
#else
    // OSG_WARN<<"operator"<<node<<std::endl;
    virtual void operator()( osg::Node* node,  osg::NodeVisitor* nv)    {
        osg::RenderInfo & ri=((osgUtil::CullVisitor*)(nv))->getRenderInfo();
        if(ri.getContextID()!=0)return;
        if(nv->getFrameStamp()->getFrameNumber()==lasttraversal)return;
        lasttraversal=nv->getFrameStamp()->getFrameNumber();
#endif
        /*  if(nv->getVisitorType()!=osg::NodeVisitor::UPDATE_VISITOR)return;
        */ //  nv->asUpdateVisitor()->get

        if(_geoms.empty()) {
#ifndef UPDATER_CAM
            nv->traverse(*node);
#endif
            return;
        }
        osg::Group*  gr  = node->asGroup();
        if(!adder.valid())
        {
            adder=new GeomLoaderAdderCallback();
            remover=new GeomLoaderRemoveCallback;
            gr->addUpdateCallback(remover);
            gr->addUpdateCallback(adder);

        }
        if(gr ->getNumChildren()>_thresremoval)
        {
            //OSG_WARN<<"removal "<<gr->getChild(0)<<std::endl;
            int curch=0,cbeg=0,cend=gr ->getNumChildren(),cinc=1;
            if(_isgeompushfront) {
                curch=gr ->getNumChildren()-1;
                cbeg=curch,cend=-1;
                cinc=-1;
            }
            osg::ref_ptr <osg::Geometry >g=gr->getChild(curch)->asGeometry();

            //  while(!g->getCreateVertexArrayStateCallback()&&curch<gr ->getNumChildren()-1)g=gr->getChild(++curch)->asGeometry();
            if(vaovis.isHackActivated())
                if(false)//!g->getVertexArray()->getUserData())
                {
                    //
                    gr ->removeChildren(curch,1);
                }
                else
                    //   for(int i=0; i<gr->getNumChildren(); ++i) {
                    for(int i=cbeg; cinc*i<cinc*cend; i+=cinc) {

//g->dirtyGLObjects();

                        g->getVertexArray()->getBufferObject()->dirty();
                        osg::Drawable* master=((osg::Drawable*)g->getVertexArray()->getUserData());
                        bool removemaster=(g==master);//!g->isCullingActive();
                        g->setCullingActive(false);
                        int bi=g->getVertexArray()->getBufferIndex();
                        osg::Geometry::ArrayList arrays;
                        osg::Geometry::DrawElementsList delmts;
                        g->getArrayList(arrays);
                        g->getDrawElementsList(delmts);
                        osg::ref_ptr<osg::BufferObject > geomvbo= arrays[0]->getBufferObject();
                        for(uint j=0; j<geomvbo->getNumBufferData(); ++j) {
                            // geomvbo->getBufferData(j)->dirty();
                            osg::ref_ptr <osg::Geometry >geom=dynamic_cast<osg::Geometry *>(geomvbo->getBufferData(j)->getUserData());
//geom->setCullingActive(false);
                            if(geom.valid())
                            {
                                /**if(j==0 && geom==g)//g is master geom so need to set next one as master
                                    removemaster=true;*/
//if(geom==g)
                                // if(removemaster)     if(j>bi) geom->setCullingActive(false);
                                // geom->dirtyBound();


                                osg::Geometry::ArrayList arrays2;
                                osg::Geometry::DrawElementsList delmts2;
                                geom->getArrayList(arrays2);
                                geom->getDrawElementsList(delmts2);
                                for(auto arr2:arrays2) {
                                    arr2->getBufferObject()-> dirty();
                                    //  for(int ij=0; ij<     arr->getBufferObject()->getNumBufferData(); ++ij) arr->getBufferObject()-> getBufferData(ij)->dirty();
                                }
                                for(auto arr2:delmts2)
                                {   arr2->getBufferObject()-> dirty();
                                    // for(int ij=0; ij<     arr->getBufferObject()->getNumBufferData(); ++ij) arr->getBufferObject()-> getBufferData(ij)->dirty();
                                }
                                /*
                                                     // releaseGLObjects(); */
                                //  geom->releaseGLObjects();
                                //geom->dirtyGLObjects();
                                // geom->osg::Drawable::releaseGLObjects();//release all VAS

                            }
                        }


                        for(auto arr:arrays)
                        {
                            //   arr->getBufferObject()->dirty();
                            arr->setBufferObject(0);
                        }
                        for(auto arr:delmts)
                        {
                            //arr->getBufferObject()->dirty();
                            arr->setBufferObject(0);// for(int j=0; j<     arr->getBufferObject()->getNumBufferData(); ++j)
                            ///arr->getBufferObject()-> getBufferData(j)->dirty();
                        }
#if 1
                        //  g->osg::Drawable::dirtyGLObjects();//release VAS
                        //  g->releaseGLObjects();//release all VAS
                        //  g->dirtyGLObjects();
#endif

                        g->setDrawCallback(new DummyDraw());
                        g->setCreateVertexArrayStateCallback(0);

                        g->getVertexArray()->setUserData(0);
#if 1
                        g->removePrimitiveSet(0,g->getNumPrimitiveSets());
                        g->setUseVertexBufferObjects(false);
                        g->setUseVertexBufferObjects(true);
                        g->setUseVertexArrayObject(true);
#endif
                        g->releaseGLObjects();//release VAS
                        //g->releaseGLObjects();
                        for(int j=0; j<geomvbo->getNumBufferData(); j++) {
                            ((osg::Drawable*)  geomvbo->getBufferData(j)->getUserData())  ->osg::Drawable:: releaseGLObjects();//destroy shared master vas
                            //                   ((osg::Drawable*)  geomvbo->getBufferData(i)->getUserData())  ->compileGLObjects(ri);
                        }
                        if(geomvbo->getBufferData(0)->getUserData()) {
                            if(geomvbo->getBufferData(0)) ((osg::Drawable*) geomvbo->getBufferData(0)->getUserData())  ->setCullingActive(false);//force master draw
                            if(geomvbo->getBufferData(0)) ((osg::Drawable*) geomvbo->getBufferData(0)->getUserData())   ->osg::Drawable::releaseGLObjects();//force master draw

                        }
                        OSG_WARN<<g->referenceCount()<<std::endl;
                        break;
                    }
            //  gr ->removeChildren(curch,1);
            remover->addToNodeList(g);

#ifndef UPDATER_CAM
            nv->traverse(*node);
#endif
            return;
        }

        std::list<osg::ref_ptr<osg::Geometry> > ::iterator it= _geoms.begin();
        int cpt=0;
        while(it!=_geoms.end()&&cpt++<_nbaddedatatime ) {


            osg::ref_ptr<osg::Geometry> newdr=(osg::Geometry*) (*it)->clone(osg::CopyOp::DEEP_COPY_ALL);

            //debug
            //  gr->setDataVariance(osg::Object::DYNAMIC);
            newdr->setDataVariance(osg::Object::STATIC);
            //    newdr->setDataVariance(osg::Object::DYNAMIC);
//  newdr  ->setComputeBoundingSphereCallback(new osg::Drawable::ComputeBoundingSphereCallback());
            newdr->setInitialBound(newdr->getBoundingBox());
            newdr->setComputeBoundingBoxCallback(new fok(newdr->getBoundingBox()));
//newdr->setCullingActive(false);
            /*   osgUtil::IndexMeshVisitor imv;
               imv.setGenerateNewIndicesOnAllGeometries(true);
               ge->accept(imv);
               imv.makeMesh(*ge->getDrawable(0)->asGeometry());*/
            newdr->accept(vaovis);
            //   ((osg::Geometry*)newdr->getVertexArray()->getUserData()) ->dirtyGLObjects();
            /*  ((osg::Geometry*)newdr->getVertexArray()->getUserData()) ->compileGLObjects(ri);
            newdr->compileGLObjects(ri);*/


            // newdr->setCullingActive(false);
            /*OSG_WARN<<"addage"<<  (*it ) ->getVertexArray()->getNumElements()<<std::endl;
            if(!_isgeompushfront)
                gr->addChild((newdr));
            else
                gr->insertChild(0,newdr);
            */


            adder->addToNodeList(newdr);
            gr->dirtyBound();
            //   newdr->setInitialBound(newdr->getBoundingBox());
//  break;
            it=_geoms.erase(it);
            //  _geoms.pop_back();it=_geoms.rbegin();
            //gr->dirtyBound();

        }
        //gr2->addChild(gr);
#ifndef UPDATER_CAM
        nv->traverse(*node);
#endif
        return;
        _geoms.clear();

    }

    mutable osg::ref_ptr<GeomLoaderAdderCallback> adder;
    mutable osg::ref_ptr<GeomLoaderRemoveCallback> remover;
};



/// This demo illustrates how VAO Sharing and basevertex draw reduce CPU draw overhead
/// when mesh number get bigger
///(for massive mesh number Indirect draw is better)
int maina(int argc, char **argv)
{

    osg::ArgumentParser args(&argc,argv);

    osgUtil::GeometryCollector geomcollector(0,osgUtil::Optimizer::ALL_OPTIMIZATIONS);

    args.getApplicationUsage()->setApplicationName(args.getApplicationName());
    args.getApplicationUsage()->setDescription(args.getApplicationName()+" is an example on how to use  bufferobject factorization+basevertex drawing in order to minimize state changes.");
    args.getApplicationUsage()->setCommandLineUsage(args.getApplicationName()+" [options] filename ...");
    args.getApplicationUsage()->addCommandLineOption("--Hmaxsize <factor>","max bufferobject size allowed (hard limit)");
    args.getApplicationUsage()->addCommandLineOption("--Smaxsize <factor>","max bufferobject size allowed (soft limit)");
    args.getApplicationUsage()->addCommandLineOption("--classic","don't use basevertex");

    unsigned int  maxsize;
    bool ispushfront=false;
    unsigned int  geomcountaddedatatime=1,geomcountabovewichweremove=1;
    while(args.read("--add",geomcountaddedatatime) ) { }
    while(args.read("--remove",geomcountabovewichweremove) ) { }
    while(args.read("--Hmaxsize",maxsize) ) {
        BufferManager::vaovis->setHardBufferSize(maxsize);
    }
    while(args.read("--Smaxsize",maxsize) ) {
        BufferManager::vaovis->setSoftBufferSize(maxsize);
    }
    if(args.read("--pushfront") ) {
        ispushfront=true;
    }
    if(args.read("--classic") ) {
        BufferManager::vaovis->setIsHackActivated(false);
    } else  BufferManager::vaovis->setIsHackActivated(true);
    osg::ref_ptr<osg::Node > loaded=osgDB::readNodeFiles(args);
    // create the model
    if(loaded.valid())
    {
        loaded->accept(geomcollector);

        osg::Geode * root=new osg::Geode;
        root->setDataVariance(osg::Object::DYNAMIC);
        //osg::Camera * root=new osg::Camera;
        GeomLoaderCB * loader=new GeomLoaderCB(geomcountabovewichweremove,geomcountaddedatatime);
        loader-> _isgeompushfront=ispushfront;
        loader->setGeometryList(            geomcollector.getGeometryList() );


        //  loaded->accept(vaovis);
        // root->setInitialDrawCallback(loader);
        osgViewer::Viewer viewer;
        viewer.addEventHandler(new osgViewer::StatsHandler);
        viewer.addEventHandler(new osgViewer::WindowSizeHandler);
        viewer.addEventHandler(new osgViewer::ThreadingHandler);
        // viewer.setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
        // add model to viewer.
        //viewer.setSceneData( loaded );

        viewer.realize();
        //root->setReferenceFrame(osg::Camera::RELATIVE_RF);
        viewer.setSceneData( root);
        viewer.init();
        viewer.frame();

        root->setUpdateCallback(new GeomLoaderRemoveCallback());
#ifndef UPDATER_CAM
        root->setCullCallback(loader);
        //     root->setUpdateCallback(loader);
#else

        viewer.getCamera()->setInitialDrawCallback(loader);
        //  viewer.getCamera()->setFinalDrawCallback(loader);
#endif

        if(args.read("--up") )
            viewer.updateTraversal();

        //viewer.setRunFrameScheme(osgViewer::ViewerBase::CONTINUOUS);
        /*  viewer.getCamera()->setCullingMode(osg::Camera:: VIEW_FRUSTUM_SIDES_CULLING|
                                             //    osg::Camera::  SMALL_FEATURE_CULLING|
                                             osg::Camera::   SHADOW_OCCLUSION_CULLING|
                                             osg::Camera::     CLUSTER_CULLING);
          viewer.getCamera()->setCullingMode(osg::Camera::   VIEW_FRUSTUM_SIDES_CULLING  |
                                             osg::Camera::NEAR_PLANE_CULLING       |
                                             osg::Camera:: FAR_PLANE_CULLING    );//
         ;*/
        /*   viewer.getCamera()->setComputeNearFarMode(osg::Camera::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);
         viewer.getCamera()->setCullingMode(osg::Camera::VIEW_FRUSTUM_CULLING | osg::Camera:: VIEW_FRUSTUM_SIDES_CULLING   |
                                            osg::Camera::NEAR_PLANE_CULLING       |
                                            osg::Camera:: FAR_PLANE_CULLING   );*/
        if(args.read("--dont") )    viewer.getCamera()->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
        loaded=0;
        viewer.getCamera()->setProjectionMatrixAsPerspective(45,1,0.1,1000);

        //viewer.getCamera()->setViewMatrixAsLookAt( osg::Vec3(0,0,20),osg::Vec3(0,0,0),osg::Vec3(0,-1,0));
        //  myFirstPersonManipulator* g=new myFirstPersonManipulator;
        osgGA::TrackballManipulator* g=new osgGA::TrackballManipulator;
        g->setIntersectTraversalMask(0);
        viewer.setCameraManipulator(new osgGA::TrackballManipulator);

        // viewer.run();//foking cam manipulator
        viewer.run();
        // viewer.releaseGLObjects();
        // return
    }
    args.getApplicationUsage()->write(std::cout,osg::ApplicationUsage::COMMAND_LINE_OPTION);

}
}
