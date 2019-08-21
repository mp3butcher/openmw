#include "lightmanager.hpp"
#include <osg/io_utils>
#include <sstream>
#include <osgUtil/CullVisitor>

#include <components/sceneutil/util.hpp>

namespace SceneUtil
{
#define MAX_LIGHTS 100
    class LightStateCache
    {
    public:
        osg::Light* lastAppliedLight[MAX_LIGHTS];
        osg::Vec4 lastAppliedLightPosition[MAX_LIGHTS];
    };

    LightStateCache* getLightStateCache(unsigned int contextid)
    {
        static std::vector<LightStateCache> cacheVector;
        if (cacheVector.size() < contextid+1)
            cacheVector.resize(contextid+1);
        return &cacheVector[contextid];
    }

    static long unsigned int cachehitmiss, cachehit;
    // transform light position by view matrix before applying lights. as uniform
    class UniformLightStateAttribute : public osg::StateAttribute
    {
    public:
        UniformLightStateAttribute() : mIndex(0) {}
        UniformLightStateAttribute(unsigned int index, const std::vector<osg::ref_ptr<osg::Light> >& lights) : mIndex(index), mLights(lights)
        {
            cachehitmiss = cachehit = 0;
            for (unsigned int pointLightIndex = 0; pointLightIndex < mLights.size(); ++pointLightIndex)
            {
                std::stringstream ss;
                ss<<(pointLightIndex+mIndex)*5;
                mLightsUniform.push_back( new osg::Uniform(osg::Uniform::FLOAT_VEC4, "lightSource["+ss.str()+"]", 5));
                osg::Light* light = mLights[pointLightIndex];

                mLightsUniform[pointLightIndex]->setElement( 0, light->getAmbient());
                mLightsUniform[pointLightIndex]->setElement( 1, light->getDiffuse());
                mLightsUniform[pointLightIndex]->setElement( 2, light->getSpecular());
                mLightsUniform[pointLightIndex]->setElement( 3, light->getPosition());
                mLightsUniform[pointLightIndex]->setElement( 4,
                        osg::Vec4(
                            light->getConstantAttenuation(),
                            light->getLinearAttenuation(),
                            light->getQuadraticAttenuation(),
                            1.f
                        )
                );
            }
        }
        UniformLightStateAttribute(const UniformLightStateAttribute& copy,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
            : osg::StateAttribute(copy,copyop), mIndex(copy.mIndex), mLights(copy.mLights) {
                mLightsUniform.resize(mLights.size());
                for (unsigned int pointLightIndex = 0; pointLightIndex < mLights.size(); ++pointLightIndex)
                    mLightsUniform[pointLightIndex] = copyop(copy.mLightsUniform[pointLightIndex]);
        }

        unsigned int getMember() const
        {
            return mIndex;
        }

        virtual bool getModeUsage(ModeUsage & usage) const
        {
            return true;
        }

        virtual int compare(const StateAttribute &sa) const
        {
            throw std::runtime_error("UniformLightStateAttribute::compare: unimplemented");
        }
//just to show that applyShaderCompositionUniform is the same as giving lower priority to UniformLightStateAttribute
#if 1
    #define USESHADERCOMPO 1
    #define LIGHTPRIORITY LIGHT
#else
    #define LIGHTPRIORITY CLAMPCOLOR
#endif
        META_StateAttribute(NifOsg, UniformLightStateAttribute, LIGHTPRIORITY)

        virtual void apply(osg::State& state) const
        {
            if (mLights.empty() )
                return;

#ifndef USESHADERCOMPO
            const osg::Program::PerContextProgram * pcp = state.getLastAppliedProgramObject();
#endif
            LightStateCache* cache = getLightStateCache(state.getContextID());
osg::Vec3d trans,s; osg::Quat q,so;
           /* state.getInitialViewMatrix().decompose(trans,q,s,so);
            OSG_WARN<<trans<<q<<std::endl;*/
            int i=0;
            auto lightuniformit=mLightsUniform.begin();
            for( auto light : mLights)
            {
                osg::Uniform * currentlightuniform=(*lightuniformit++);
                osg::Light* current = cache->lastAppliedLight[i+mIndex];
                osg::Vec4 pos2, pos = cache->lastAppliedLightPosition[i+mIndex];

                currentlightuniform->getElement(3,pos2);
                if(current!=mLights[i].get() ||pos2!=pos|| (current&&current->compare(*light.get()))!=0)
                {
                    cachehitmiss++;
                    //if(pos2!=pos)OSG_WARN<<"cachehit initialmodelview"<<std::endl;
                    osg::Vec3 pos(light->getPosition().x(), light->getPosition().y(), light->getPosition().z());
                    osg::Vec4 tempos = osg::Vec4(pos* state.getInitialViewMatrix(), light->getPosition().w());
                    currentlightuniform->setElement( 3, light->getPosition()* state.getInitialViewMatrix());
                    currentlightuniform->setElement( 0, light->getAmbient());
                    currentlightuniform->setElement( 1, light->getDiffuse());
                    currentlightuniform->setElement( 2, light->getSpecular());
                    currentlightuniform->dirty();

#ifdef USESHADERCOMPO
                    state.applyShaderCompositionUniform(currentlightuniform, osg::StateAttribute::ON |osg::StateAttribute::OVERRIDE );
#else
                    if(pcp) mLightsUniform[i]->apply(state.get<osg::GLExtensions>(), state.getUniformLocation( currentlightuniform->getNameID()));
#endif
                    cache->lastAppliedLight[i+mIndex] = mLights[i].get();
                    cache->lastAppliedLightPosition[i+mIndex] = tempos;
                }
                else cachehit++;

                ++i;
            }
             OSG_WARN<<"cahce used"<<state.getFrameStamp()->getFrameNumber()<<" "<<(float)cachehit/(float)(cachehitmiss+cachehit)<<std::endl;
        }

        std::vector<osg::ref_ptr<osg::Uniform> >& getLightUniforms() { return mLightsUniform; }
    private:
        unsigned int mIndex;        
        std::vector<osg::ref_ptr<osg::Light> > mLights;
        std::vector<osg::ref_ptr<osg::Uniform> > mLightsUniform;
    };

    LightManager* findLightManager(const osg::NodePath& path)
    {
        for (unsigned int i=0;i<path.size(); ++i)
        {
            if (LightManager* lightManager = dynamic_cast<LightManager*>(path[i]))
                return lightManager;
        }
        return nullptr;
    }

    // Set on a LightSource. Adds the light source to its light manager for the current frame.
    // This allows us to keep track of the current lights in the scene graph without tying creation & destruction to the manager.
    class CollectLightCallback : public osg::NodeCallback
    {
    public:
        CollectLightCallback()
            : mLightManager(0) { }

        CollectLightCallback(const CollectLightCallback& copy, const osg::CopyOp& copyop)
            : osg::NodeCallback(copy, copyop)
            , mLightManager(0) { }

        META_Object(SceneUtil, SceneUtil::CollectLightCallback)

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            if (!mLightManager)
            {
                mLightManager = findLightManager(nv->getNodePath());

                if (!mLightManager)
                    throw std::runtime_error("can't find parent LightManager");
            }

            mLightManager->addLight(static_cast<LightSource*>(node), osg::computeLocalToWorld(nv->getNodePath()), nv->getTraversalNumber());

            traverse(node, nv);
        }

    private:
        LightManager* mLightManager;
    };

    // Set on a LightManager. Clears the data from the previous frame.
    class LightManagerUpdateCallback : public osg::NodeCallback
    {
    public:
        LightManagerUpdateCallback()
            { }

        LightManagerUpdateCallback(const LightManagerUpdateCallback& copy, const osg::CopyOp& copyop)
            : osg::NodeCallback(copy, copyop)
            { }

        META_Object(SceneUtil, LightManagerUpdateCallback)

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            LightManager* lightManager = static_cast<LightManager*>(node);
            lightManager->update();

            traverse(node, nv);
        }
    };

    LightManager::LightManager(unsigned int maxlights)
        : mStartLight(0)
        , mLightingMask(~0u)
        , mMaxlights(maxlights)
    {
        setUpdateCallback(new LightManagerUpdateCallback);
        for (unsigned int i=0; i<mMaxlights; ++i)
            mDummies.push_back(new UniformLightStateAttribute(i, std::vector<osg::ref_ptr<osg::Light> >()));
    }

    LightManager::LightManager(const LightManager &copy, const osg::CopyOp &copyop)
        : osg::Group(copy, copyop)
        , mStartLight(copy.mStartLight)
        , mLightingMask(copy.mLightingMask)
        , mMaxlights(copy.mMaxlights)
    {

    }

    void LightManager::setLightingMask(unsigned int mask)
    {
        mLightingMask = mask;
    }

    unsigned int LightManager::getLightingMask() const
    {
        return mLightingMask;
    }

    void LightManager::update()
    {
        mLights.clear();
        mLightsInViewSpace.clear();

        // do an occasional cleanup for orphaned lights
        for (int i=0; i<2; ++i)
        {
            if (mStateSetCache[i].size() > 5000)
                mStateSetCache[i].clear();
        }
    }

    void LightManager::addLight(LightSource* lightSource, const osg::Matrixf& worldMat, unsigned int frameNum)
    {
        LightSourceTransform l;
        l.mLightSource = lightSource;
        l.mWorldMatrix = worldMat;
        lightSource->getLight(frameNum)->setPosition(osg::Vec4f(worldMat.getTrans().x(),
                                                        worldMat.getTrans().y(),
                                                        worldMat.getTrans().z(), 1.f));
        mLights.push_back(l);
    }

    /* similar to the boost::hash_combine */
    template <class T>
    inline void hash_combine(std::size_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }

    osg::ref_ptr<osg::StateSet> LightManager::getLightListStateSet(const LightList &lightList, unsigned int frameNum)
    {
        // possible optimization: return a StateSet containing all requested lights plus some extra lights (if a suitable one exists)
        size_t hash = 0;
        for (unsigned int i=0; i<lightList.size();++i)
            hash_combine(hash, lightList[i]->mLightSource->getId());

        LightStateSetMap& stateSetCache = mStateSetCache[frameNum%2];

        LightStateSetMap::iterator found = stateSetCache.find(hash);
        if (found != stateSetCache.end())
            return found->second;
        else
        {
            osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet();
            std::vector<osg::ref_ptr<osg::Light> > lights;
            lights.reserve(lightList.size());
            for (unsigned int i=0; i<lightList.size();++i)
                lights.push_back(lightList[i]->mLightSource->getLight(frameNum));

            //TODO: sun
            // the first light state attribute handles the actual state setting for all lights
            // it's best to batch these up so that we don't need to touch the modelView matrix more than necessary
            // don't use setAttributeAndModes, that does not support light indices!
            UniformLightStateAttribute* unilights=new UniformLightStateAttribute(mStartLight, std::move(lights));
            stateset->setAttribute(unilights, osg::StateAttribute::ON| osg::StateAttribute::OVERRIDE);

            // need to push some dummy attributes to ensure proper state tracking
            // lights need to reset to their default when the StateSet is popped
            for (unsigned int i=1; i<lightList.size(); ++i){
                stateset->setAttribute(mDummies[i+mStartLight].get(), osg::StateAttribute::ON);
                for( auto unilight : ((UniformLightStateAttribute* )mDummies[i+mStartLight].get())->getLightUniforms()){
                  //  stateset->addUniform(unilight, osg::StateAttribute::ON| osg::StateAttribute::OVERRIDE);
                }
            }
            for( auto unilight : unilights->getLightUniforms())
            {
                stateset->addUniform(unilight, osg::StateAttribute::ON| osg::StateAttribute::OVERRIDE);
            }
            stateset->addUniform(new osg::Uniform("lightCount",(int)lightList.size()+1));

            stateSetCache.emplace(hash, stateset);
            return stateset;
        }
    }

    const std::vector<LightManager::LightSourceTransform>& LightManager::getLights() const
    {
        return mLights;
    }

    const std::vector<LightManager::LightSourceViewBound>& LightManager::getLightsInViewSpace(osg::Camera *camera, const osg::RefMatrix* viewMatrix)
    {
        osg::observer_ptr<osg::Camera> camPtr (camera);
        std::map<osg::observer_ptr<osg::Camera>, LightSourceViewBoundCollection>::iterator it = mLightsInViewSpace.find(camPtr);

        if (it == mLightsInViewSpace.end())
        {
            it = mLightsInViewSpace.insert(std::make_pair(camPtr, LightSourceViewBoundCollection())).first;

            for (std::vector<LightSourceTransform>::iterator lightIt = mLights.begin(); lightIt != mLights.end(); ++lightIt)
            {
                osg::Matrixf worldViewMat = lightIt->mWorldMatrix * (*viewMatrix);
                osg::BoundingSphere viewBound = osg::BoundingSphere(osg::Vec3f(0,0,0), lightIt->mLightSource->getRadius());
                transformBoundingSphere(worldViewMat, viewBound);

                LightSourceViewBound l;
                l.mLightSource = lightIt->mLightSource;
                l.mViewBound = viewBound;
                it->second.push_back(l);
            }
        }
        return it->second;
    }

    class DisableLight : public osg::StateAttribute
    {
    public:
        DisableLight() : mIndex(0)
        {
            std::stringstream ss;
            ss<<mIndex*5;
            mLightUniform = new osg::Uniform(osg::Uniform::FLOAT_VEC4, "lightSource["+ss.str()+"]", 5);
            mLightUniform->setElement(0, mnullptr);
            mLightUniform->setElement(1, mnullptr);
            mLightUniform->setElement(2, mnullptr);
            mLightUniform->setElement(3, mnullptr);
            mLightUniform->setElement(4, mnullptr);
        }
        DisableLight(int index) : mIndex(index)
        {
            std::stringstream ss;
            ss<<mIndex*5;
            mLightUniform = new osg::Uniform(osg::Uniform::FLOAT_VEC4, "lightSource["+ss.str()+"]", 5);
            mLightUniform->setElement(0, mnullptr);
            mLightUniform->setElement(1, mnullptr);
            mLightUniform->setElement(2, mnullptr);
            mLightUniform->setElement(3, mnullptr);
            mLightUniform->setElement(4, mnullptr);
        }

        DisableLight(const DisableLight& copy,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
            : osg::StateAttribute(copy,copyop), mIndex(copy.mIndex) {
            mLightUniform = copyop(copy.mLightUniform);
        }

        virtual osg::Object* cloneType() const { return new DisableLight(mIndex); }
        virtual osg::Object* clone(const osg::CopyOp& copyop) const { return new DisableLight(*this,copyop); }
        virtual bool isSameKindAs(const osg::Object* obj) const { return dynamic_cast<const DisableLight *>(obj)!=nullptr; }
        virtual const char* libraryName() const { return "SceneUtil"; }
        virtual const char* className() const { return "DisableLight"; }
        virtual Type getType() const { return LIGHTPRIORITY; }

        unsigned int getMember() const
        {
            return mIndex;
        }

        virtual bool getModeUsage(ModeUsage & usage) const
        {
            return true;
        }

        virtual int compare(const StateAttribute &sa) const
        {
            throw std::runtime_error("DisableLight::compare: unimplemented");
        }

        virtual void apply(osg::State& state) const
        {
#ifdef USESHADERCOMPO
            state.applyShaderCompositionUniform(mLightUniform,osg::StateAttribute::ON |osg::StateAttribute::OVERRIDE );
#else
            const osg::Program::PerContextProgram * pcp = state.getLastAppliedProgramObject();
            if(pcp) mLightUniform->apply(state.get<osg::GLExtensions>(), state.getUniformLocation(mLightUniform->getNameID()));
#endif
            LightStateCache* cache = getLightStateCache(state.getContextID());
            cache->lastAppliedLight[mIndex] = nullptr;
        }        
        osg::Uniform* getLightUniform() { return mLightUniform; }
    private:
        unsigned int mIndex;
        osg::Vec4f mnullptr;
        osg::ref_ptr<osg::Uniform> mLightUniform;
    };

    void LightManager::setStartLight(int start)
    {
        mStartLight = start;

        // Set default light state to zero
        // This is necessary because shaders don't respect glDisable(GL_LIGHTX) so in addition to disabling
        // we'll have to set a light state that has no visible effect
        for (unsigned int i=start; i<mMaxlights; ++i)
        {
            osg::ref_ptr<DisableLight> defaultLight (new DisableLight(i));
            getOrCreateStateSet()->addUniform(defaultLight->getLightUniform());
            getOrCreateStateSet()->setAttributeAndModes(defaultLight, osg::StateAttribute::OFF);
        }
    }

    int LightManager::getStartLight() const
    {
        return mStartLight;
    }

    static int sLightId = 0;

    LightSource::LightSource()
        : mRadius(0.f)
    {
        setUpdateCallback(new CollectLightCallback);
        mId = sLightId++;
    }

    LightSource::LightSource(const LightSource &copy, const osg::CopyOp &copyop)
        : osg::Node(copy, copyop)
        , mRadius(copy.mRadius)
    {
        mId = sLightId++;

        for (int i=0; i<2; ++i)
            mLight[i] = new osg::Light(*copy.mLight[i].get(), copyop);
    }


    bool sortLights (const LightManager::LightSourceViewBound* left, const LightManager::LightSourceViewBound* right)
    {
        return left->mViewBound.center().length2() - left->mViewBound.radius2()*81 < right->mViewBound.center().length2() - right->mViewBound.radius2()*81;
    }

    void LightListCallback::operator()(osg::Node *node, osg::NodeVisitor *nv)
    {
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);

        bool pushedState = pushLightState(node, cv);
        traverse(node, nv);
        if (pushedState)
            cv->popStateSet();
    }

    bool LightListCallback::pushLightState(osg::Node *node, osgUtil::CullVisitor *cv)
    {
        if (!mLightManager)
        {
            mLightManager = findLightManager(cv->getNodePath());
            if (!mLightManager)
                return false;
        }

        if (!(cv->getTraversalMask() & mLightManager->getLightingMask()))
            return false;

        // Possible optimizations:
        // - cull list of lights by the camera frustum
        // - organize lights in a quad tree


        // update light list if necessary
        // makes sure we don't update it more than once per frame when rendering with multiple cameras
        if (mLastFrameNumber != cv->getTraversalNumber())
        {
            mLastFrameNumber = cv->getTraversalNumber();

            // Don't use Camera::getViewMatrix, that one might be relative to another camera!
            const osg::RefMatrix* viewMatrix = cv->getCurrentRenderStage()->getInitialViewMatrix();
            const std::vector<LightManager::LightSourceViewBound>& lights = mLightManager->getLightsInViewSpace(cv->getCurrentCamera(), viewMatrix);

            // get the node bounds in view space
            // NB do not node->getBound() * modelView, that would apply the node's transformation twice
            osg::BoundingSphere nodeBound;
            osg::Transform* transform = node->asTransform();
            if (transform)
            {
                for (unsigned int i=0; i<transform->getNumChildren(); ++i)
                    nodeBound.expandBy(transform->getChild(i)->getBound());
            }
            else
                nodeBound = node->getBound();
            osg::Matrixf mat = *cv->getModelViewMatrix();
            transformBoundingSphere(mat, nodeBound);

            mLightList.clear();
            for (unsigned int i=0; i<lights.size(); ++i)
            {
                const LightManager::LightSourceViewBound& l = lights[i];

                if (mIgnoredLightSources.count(l.mLightSource))
                    continue;

                if (l.mViewBound.intersects(nodeBound))
                    mLightList.push_back(&l);
            }
        }
        if (!mLightList.empty())
        {
            unsigned int maxLights = static_cast<unsigned int> (mLightManager->getMaxLightCount() - mLightManager->getStartLight());

            osg::StateSet* stateset = nullptr;

            if (mLightList.size() > maxLights)
            {
                // remove lights culled by this camera
                LightManager::LightList lightList = mLightList;
                for (LightManager::LightList::iterator it = lightList.begin(); it != lightList.end() && lightList.size() > maxLights; )
                {
                    osg::CullStack::CullingStack& stack = cv->getModelViewCullingStack();

                    osg::BoundingSphere bs = (*it)->mViewBound;
                    bs._radius = bs._radius*2;
                    osg::CullingSet& cullingSet = stack.front();
                    if (cullingSet.isCulled(bs))
                    {
                        it = lightList.erase(it);
                        continue;
                    }
                    else
                        ++it;
                }

                if (lightList.size() > maxLights)
                {
                    // sort by proximity to camera, then get rid of furthest away lights
                    std::sort(lightList.begin(), lightList.end(), sortLights);
                    while (lightList.size() > maxLights)
                        lightList.pop_back();
                }
                stateset = mLightManager->getLightListStateSet(lightList, cv->getTraversalNumber());
            }
            else
                stateset = mLightManager->getLightListStateSet(mLightList, cv->getTraversalNumber());


            cv->pushStateSet(stateset);
            return true;
        }
        return false;
    }

}
