#include "engine.h"

#include "aa.h"
#include "radiancehints.h"
#include "renderwindow.h"

VAR(oqdynent, 0, 1, 1);
VAR(animationinterpolationtime, 0, 200, 1000);

int numanims; //set by game at runtime
std::vector<std::string> animnames; //set by game at runtime

model *loadingmodel = NULL;

//need the above vars inited before these headers will load properly

#include "model/ragdoll.h"
#include "model/animmodel.h"
#include "model/vertmodel.h"
#include "model/skelmodel.h"

#include "world/hitzone.h"

model *loadmapmodel(int n)
{
    if(mapmodels.inrange(n))
    {
        model *m = mapmodels[n].m;
        return m ? m : loadmodel(NULL, n);
    }
    return NULL;
}

static model *(__cdecl *modeltypes[MDL_NumMDLTypes])(const char *);

static int addmodeltype(int type, model *(__cdecl *loader)(const char *))
{
    modeltypes[type] = loader;
    return type;
}

#define MODELTYPE(modeltype, modelclass) \
static model *loadmodel_##modelclass(const char *filename) \
{ \
    return new modelclass(filename); \
} \
static int dummy_##modelclass = addmodeltype((modeltype), loadmodel_##modelclass);

//need the above macros & fxns inited before these headers will load properly
#include "model/md5.h"
#include "model/obj.h"

MODELTYPE(MDL_MD5, md5);
MODELTYPE(MDL_OBJ, obj);

static inline void checkmdl()
{
    if(!loadingmodel)
    {
        conoutf(Console_Error, "not loading a model");
        return;
    }
}

void mdlcullface(int *cullface)
{
    checkmdl();
    loadingmodel->setcullface(*cullface);
}
COMMAND(mdlcullface, "i");

void mdlcolor(float *r, float *g, float *b)
{
    checkmdl();
    loadingmodel->setcolor(vec(*r, *g, *b));
}
COMMAND(mdlcolor, "fff");

void mdlcollide(int *collide)
{
    checkmdl();
    loadingmodel->collide = *collide!=0 ? (loadingmodel->collide ? loadingmodel->collide : Collide_OrientedBoundingBox) : Collide_None;
}
COMMAND(mdlcollide, "i");

void mdlellipsecollide(int *collide)
{
    checkmdl();
    loadingmodel->collide = *collide!=0 ? Collide_Ellipse : Collide_None;
}
COMMAND(mdlellipsecollide, "i");

void mdltricollide(char *collide)
{
    checkmdl();
    DELETEA(loadingmodel->collidemodel);
    char *end = NULL;
    int val = strtol(collide, &end, 0);
    if(*end)
    {
        val = 1;
        loadingmodel->collidemodel = newstring(collide);
    }
    loadingmodel->collide = val ? Collide_TRI : Collide_None;
}
COMMAND(mdltricollide, "s");

void mdlspec(float *percent)
{
    checkmdl();
    float spec = *percent > 0 ? *percent/100.0f : 0.0f;
    loadingmodel->setspec(spec);
}
COMMAND(mdlspec, "f");

void mdlgloss(int *gloss)
{
    checkmdl();
    loadingmodel->setgloss(std::clamp(*gloss, 0, 2));
}
COMMAND(mdlgloss, "i");

void mdlalphatest(float *cutoff)
{
    checkmdl();
    loadingmodel->setalphatest(max(0.0f, min(1.0f, *cutoff)));
}
COMMAND(mdlalphatest, "f");

void mdldepthoffset(int *offset)
{
    checkmdl();
    loadingmodel->depthoffset = *offset!=0;
}
COMMAND(mdldepthoffset, "i");

void mdlglow(float *percent, float *delta, float *pulse)
{
    checkmdl();
    float glow = *percent > 0 ? *percent/100.0f : 0.0f,
          glowdelta = *delta/100.0f,
          glowpulse = *pulse > 0 ? *pulse/1000.0f : 0;
    glowdelta -= glow;
    loadingmodel->setglow(glow, glowdelta, glowpulse);
}
COMMAND(mdlglow, "fff");

void mdlfullbright(float *fullbright)
{
    checkmdl();
    loadingmodel->setfullbright(*fullbright);
}
COMMAND(mdlfullbright, "f");

void mdlshader(char *shader)
{
    checkmdl();
    loadingmodel->setshader(lookupshaderbyname(shader));
}
COMMAND(mdlshader, "s");

void mdlspin(float *yaw, float *pitch, float *roll)
{
    checkmdl();
    loadingmodel->spinyaw = *yaw;
    loadingmodel->spinpitch = *pitch;
    loadingmodel->spinroll = *roll;
}
COMMAND(mdlspin, "fff");

void mdlscale(float *percent)
{
    checkmdl();
    float scale = *percent > 0 ? *percent/100.0f : 1.0f;
    loadingmodel->scale = scale;
}
COMMAND(mdlscale, "f");

void mdltrans(float *x, float *y, float *z)
{
    checkmdl();
    loadingmodel->translate = vec(*x, *y, *z);
}
COMMAND(mdltrans, "fff");

void mdlyaw(float *angle)
{
    checkmdl();
    loadingmodel->offsetyaw = *angle;
}
COMMAND(mdlyaw, "f");

void mdlpitch(float *angle)
{
    checkmdl();
    loadingmodel->offsetpitch = *angle;
}
COMMAND(mdlpitch, "f");

void mdlroll(float *angle)
{
    checkmdl();
    loadingmodel->offsetroll = *angle;
}
COMMAND(mdlroll, "f");

void mdlshadow(int *shadow)
{
    checkmdl();
    loadingmodel->shadow = *shadow!=0;
}
COMMAND(mdlshadow, "i");

void mdlalphashadow(int *alphashadow)
{
    checkmdl();
    loadingmodel->alphashadow = *alphashadow!=0;
}
COMMAND(mdlalphashadow, "i");

void mdlbb(float *rad, float *h, float *eyeheight)
{
    checkmdl();
    loadingmodel->collidexyradius = *rad;
    loadingmodel->collideheight = *h;
    loadingmodel->eyeheight = *eyeheight;
}
COMMAND(mdlbb, "fff");

void mdlextendbb(float *x, float *y, float *z)
{
    checkmdl();
    loadingmodel->bbextend = vec(*x, *y, *z);
}
COMMAND(mdlextendbb, "fff");

void mdlname()
{
    checkmdl();
    result(loadingmodel->name);
}
COMMAND(mdlname, "");

#define CHECK_RAGDOLL \
    checkmdl(); \
    if(!loadingmodel->skeletal()) \
    { \
        conoutf(Console_Error, "not loading a skeletal model"); \
        return; \
    } \
    skelmodel *m = (skelmodel *)loadingmodel; \
    if(m->parts.empty()) \
    { \
        return; \
    } \
    skelmodel::skelmeshgroup *meshes = (skelmodel::skelmeshgroup *)m->parts.last()->meshes; \
    if(!meshes) \
    { \
        return; \
    } \
    skelmodel::skeleton *skel = meshes->skel; \
    if(!skel->ragdoll) \
    { \
        skel->ragdoll = new ragdollskel; \
    } \
    ragdollskel *ragdoll = skel->ragdoll; \
    if(ragdoll->loaded) \
    { \
        return; \
    }


void rdvert(float *x, float *y, float *z, float *radius)
{
    CHECK_RAGDOLL;
    ragdollskel::vert &v = ragdoll->verts.add();
    v.pos = vec(*x, *y, *z);
    v.radius = *radius > 0 ? *radius : 1;
}
COMMAND(rdvert, "ffff");

void rdeye(int *v)
{
    CHECK_RAGDOLL;
    ragdoll->eye = *v;
}
COMMAND(rdeye, "i");

void rdtri(int *v1, int *v2, int *v3)
{
    CHECK_RAGDOLL;
    ragdollskel::tri &t = ragdoll->tris.add();
    t.vert[0] = *v1;
    t.vert[1] = *v2;
    t.vert[2] = *v3;
}
COMMAND(rdtri, "iii");

void rdjoint(int *n, int *t, int *v1, int *v2, int *v3)
{
    CHECK_RAGDOLL;
    if(*n < 0 || *n >= skel->numbones)
    {
        return;
    }
    ragdollskel::joint &j = ragdoll->joints.add();
    j.bone = *n;
    j.tri = *t;
    j.vert[0] = *v1;
    j.vert[1] = *v2;
    j.vert[2] = *v3;
}
COMMAND(rdjoint, "iibbb");

void rdlimitdist(int *v1, int *v2, float *mindist, float *maxdist)
{
    CHECK_RAGDOLL;
    ragdollskel::distlimit &d = ragdoll->distlimits.add();
    d.vert[0] = *v1;
    d.vert[1] = *v2;
    d.mindist = *mindist;
    d.maxdist = max(*maxdist, *mindist);
}
COMMAND(rdlimitdist, "iiff");

void rdlimitrot(int *t1, int *t2, float *maxangle, float *qx, float *qy, float *qz, float *qw)
{
    CHECK_RAGDOLL;
    ragdollskel::rotlimit &r = ragdoll->rotlimits.add();
    r.tri[0] = *t1;
    r.tri[1] = *t2;
    r.maxangle = *maxangle * RAD;
    r.maxtrace = 1 + 2*cos(r.maxangle);
    r.middle = matrix3(quat(*qx, *qy, *qz, *qw));
}
COMMAND(rdlimitrot, "iifffff");

void rdanimjoints(int *on)
{
    CHECK_RAGDOLL;
    ragdoll->animjoints = *on!=0;
}
COMMAND(rdanimjoints, "i");

// mapmodels

vector<mapmodelinfo> mapmodels;
static const char * const mmprefix = "mapmodel/";
static const int mmprefixlen = strlen(mmprefix);

void mapmodel(char *name)
{
    mapmodelinfo &mmi = mapmodels.add();
    if(name[0])
    {
        formatstring(mmi.name, "%s%s", mmprefix, name);
    }
    else
    {
        mmi.name[0] = '\0';
    }
    mmi.m = mmi.collide = NULL;
}

void mapmodelreset(int *n)
{
    if(!(identflags&Idf_Overridden) && !allowediting)
    {
        return;
    }
    mapmodels.shrink(std::clamp(*n, 0, mapmodels.length()));
}

const char *mapmodelname(int i)
{
    return mapmodels.inrange(i) ? mapmodels[i].name : NULL;
}

ICOMMAND(mmodel, "s", (char *name), mapmodel(name));
COMMAND(mapmodel, "s");
COMMAND(mapmodelreset, "i");
ICOMMAND(mapmodelname, "ii", (int *index, int *prefix),
{
    if(mapmodels.inrange(*index))
    {
        result(mapmodels[*index].name[0] ? mapmodels[*index].name + (*prefix ? 0 : mmprefixlen) : "");
    }
});
ICOMMAND(mapmodelloaded, "i", (int *index), { intret(mapmodels.inrange(*index) && mapmodels[*index].m ? 1 : 0); });
ICOMMAND(nummapmodels, "", (), { intret(mapmodels.length()); });

// model registry

hashnameset<model *> models;
vector<const char *> preloadmodels;
hashset<char *> failedmodels;

void preloadmodel(const char *name)
{
    if(!name || !name[0] || models.access(name) || preloadmodels.htfind(name) >= 0)
    {
        return;
    }
    preloadmodels.add(newstring(name));
}

void flushpreloadedmodels(bool msg)
{
    for(int i = 0; i < preloadmodels.length(); i++)
    {
        loadprogress = static_cast<float>(i+1)/preloadmodels.length();
        model *m = loadmodel(preloadmodels[i], -1, msg);
        if(!m)
        {
            if(msg)
            {
                conoutf(Console_Warn, "could not load model: %s", preloadmodels[i]);
            }
        }
        else
        {
            m->preloadmeshes();
            m->preloadshaders();
        }
    }
    preloadmodels.deletearrays();
    loadprogress = 0;
}

void preloadusedmapmodels(bool msg, bool bih)
{
    vector<extentity *> &ents = entities::getents();
    std::vector<int> used;
    for(int i = 0; i < ents.length(); i++)
    {
        extentity &e = *ents[i];
        if(e.type==EngineEnt_Mapmodel && e.attr1 >= 0 && std::find(used.begin(), used.end(), e.attr1) != used.end() )
        {
            used.push_back(e.attr1);
        }
    }

    vector<const char *> col;
    for(uint i = 0; i < used.size(); i++)
    {
        loadprogress = static_cast<float>(i+1)/used.size();
        int mmindex = used[i];
        if(!mapmodels.inrange(mmindex))
        {
            if(msg)
            {
                conoutf(Console_Warn, "could not find map model: %d", mmindex);
            }
            continue;
        }
        mapmodelinfo &mmi = mapmodels[mmindex];
        if(!mmi.name[0])
        {
            continue;
        }
        model *m = loadmodel(NULL, mmindex, msg);
        if(!m)
        {
            if(msg)
            {
                conoutf(Console_Warn, "could not load map model: %s", mmi.name);
            }
        }
        else
        {
            if(bih)
            {
                m->preloadBIH();
            }
            else if(m->collide == Collide_TRI && !m->collidemodel && m->bih)
            {
                m->setBIH();
            }
            m->preloadmeshes();
            m->preloadshaders();
            if(m->collidemodel && col.htfind(m->collidemodel) < 0)
            {
                col.add(m->collidemodel);
            }
        }
    }

    for(int i = 0; i < col.length(); i++)
    {
        loadprogress = static_cast<float>(i+1)/col.length();
        model *m = loadmodel(col[i], -1, msg);
        if(!m)
        {
            if(msg)
            {
                conoutf(Console_Warn, "could not load collide model: %s", col[i]);
            }
        }
        else if(!m->bih)
        {
            m->setBIH();
        }
    }

    loadprogress = 0;
}

model *loadmodel(const char *name, int i, bool msg)
{
    if(!name)
    {
        if(!mapmodels.inrange(i))
        {
            return NULL;
        }
        mapmodelinfo &mmi = mapmodels[i];
        if(mmi.m)
        {
            return mmi.m;
        }
        name = mmi.name;
    }
    model **mm = models.access(name);
    model *m;
    if(mm)
    {
        m = *mm;
    }
    else
    {
        if(!name[0] || loadingmodel || failedmodels.find(name, NULL))
        {
            return NULL;
        }
        if(msg)
        {
            DEF_FORMAT_STRING(filename, "media/model/%s", name);
            renderprogress(loadprogress, filename);
        }
        for(int i = 0; i < MDL_NumMDLTypes; ++i)
        {
            m = modeltypes[i](name);
            if(!m)
            {
                continue;
            }
            loadingmodel = m;
            if(m->load())
            {
                break;
            }
            DELETEP(m);
        }
        loadingmodel = NULL;
        if(!m)
        {
            failedmodels.add(newstring(name));
            return NULL;
        }
        models.access(m->name, m);
    }
    if(mapmodels.inrange(i) && !mapmodels[i].m)
    {
        mapmodels[i].m = m;
    }
    return m;
}

void clear_models()
{
    ENUMERATE(models, model *, m, delete m);
}

void cleanupmodels()
{
    ENUMERATE(models, model *, m, m->cleanup());
}

void clearmodel(char *name)
{
    model *m = models.find(name, NULL);
    if(!m)
    {
        conoutf("model %s is not loaded", name);
        return;
    }
    for(int i = 0; i < mapmodels.length(); i++)
    {
        mapmodelinfo &mmi = mapmodels[i];
        if(mmi.m == m)
        {
            mmi.m = NULL;
        }
        if(mmi.collide == m)
        {
            mmi.collide = NULL;
        }
    }
    models.remove(name);
    m->cleanup();
    delete m;
    conoutf("cleared model %s", name);
}

COMMAND(clearmodel, "s");

bool modeloccluded(const vec &center, float radius)
{
    ivec bbmin(vec(center).sub(radius)),
         bbmax(vec(center).add(radius+1));
    return bboccluded(bbmin, bbmax);
}

struct batchedmodel
{
    vec pos, center;
    float radius, yaw, pitch, roll, sizescale;
    vec4 colorscale;
    int anim, basetime, basetime2, flags, attached;
    union
    {
        int visible;
        int culled;
    };
    dynent *d;
    int next;
};
struct modelbatch
{
    model *m;
    int flags, batched;
};
static std::vector<batchedmodel> batchedmodels;
static vector<modelbatch> batches;
static vector<modelattach> modelattached;

void resetmodelbatches()
{
    batchedmodels.clear();
    batches.setsize(0);
    modelattached.setsize(0);
}

void addbatchedmodel(model *m, batchedmodel &bm, int idx)
{
    modelbatch *b = NULL;
    if(batches.inrange(m->batch))
    {
        b = &batches[m->batch];
        if(b->m == m && (b->flags & Model_Mapmodel) == (bm.flags & Model_Mapmodel))
        {
            goto foundbatch; //skip some shit
        }
    }
    m->batch = batches.length();
    b = &batches.add();
    b->m = m;
    b->flags = 0;
    b->batched = -1;

foundbatch:
    b->flags |= bm.flags;
    bm.next = b->batched;
    b->batched = idx;
}

static inline void renderbatchedmodel(model *m, const batchedmodel &b)
{
    modelattach *a = NULL;
    if(b.attached>=0)
    {
        a = &modelattached[b.attached];
    }
    int anim = b.anim;
    if(shadowmapping > ShadowMap_Reflect)
    {
        anim |= Anim_NoSkin;
    }
    else
    {
        if(b.flags&Model_FullBright)
        {
            anim |= Anim_FullBright;
        }
    }

    m->render(anim, b.basetime, b.basetime2, b.pos, b.yaw, b.pitch, b.roll, b.d, a, b.sizescale, b.colorscale);
}

//ratio between model size and distance at which to cull: at 200, model must be 200 times smaller than distance to model
VAR(maxmodelradiusdistance, 10, 200, 1000);

static inline void enablecullmodelquery()
{
    startbb();
}

static inline void rendercullmodelquery(model *m, dynent *d, const vec &center, float radius)
{
    if(fabs(camera1->o.x-center.x) < radius+1 &&
       fabs(camera1->o.y-center.y) < radius+1 &&
       fabs(camera1->o.z-center.z) < radius+1)
    {
        d->query = NULL;
        return;
    }
    d->query = newquery(d);
    if(!d->query)
    {
        return;
    }
    startquery(d->query);
    int br = static_cast<int>(radius*2)+1;
    drawbb(ivec(static_cast<float>(center.x-radius), static_cast<float>(center.y-radius), static_cast<float>(center.z-radius)), ivec(br, br, br));
    endquery();
}

static inline void disablecullmodelquery()
{
    endbb();
}

static inline int cullmodel(model *m, const vec &center, float radius, int flags, dynent *d = NULL)
{
    if(flags&Model_CullDist && (center.dist(camera1->o) / radius) > maxmodelradiusdistance)
    {
        return Model_CullDist;
    }
    if(flags&Model_CullVFC && isfoggedsphere(radius, center))
    {
        return Model_CullVFC;
    }
    if(flags&Model_CullOccluded && modeloccluded(center, radius))
    {
        return Model_CullOccluded;
    }
    else if(flags&Model_CullQuery && d->query && d->query->owner==d && checkquery(d->query))
    {
        return Model_CullQuery;
    }
    return 0;
}

static inline int shadowmaskmodel(const vec &center, float radius)
{
    switch(shadowmapping)
    {
        case ShadowMap_Reflect:
            return calcspherersmsplits(center, radius);
        case ShadowMap_CubeMap:
        {
            vec scenter = vec(center).sub(shadoworigin);
            float sradius = radius + shadowradius;
            if(scenter.squaredlen() >= sradius*sradius)
            {
                return 0;
            }
            return calcspheresidemask(scenter, radius, shadowbias);
        }
        case ShadowMap_Cascade:
        {
            return calcspherecsmsplits(center, radius);
        }
        case ShadowMap_Spot:
        {
            vec scenter = vec(center).sub(shadoworigin);
            float sradius = radius + shadowradius;
            return scenter.squaredlen() < sradius*sradius && sphereinsidespot(shadowdir, shadowspot, scenter, radius) ? 1 : 0;
        }
    }
    return 0;
}

void shadowmaskbatchedmodels(bool dynshadow)
{
    for(uint i = 0; i < batchedmodels.size(); i++)
    {
        batchedmodel &b = batchedmodels[i];
        if(b.flags&(Model_Mapmodel | Model_NoShadow))
        {
            break;
        }
        b.visible = dynshadow && (b.colorscale.a >= 1 || b.flags&(Model_OnlyShadow | Model_ForceShadow)) ? shadowmaskmodel(b.center, b.radius) : 0;
    }
}

int batcheddynamicmodels()
{
    int visible = 0;
    for(uint i = 0; i < batchedmodels.size(); i++)
    {
        batchedmodel &b = batchedmodels[i];
        if(b.flags&Model_Mapmodel)
        {
            break;
        }
        visible |= b.visible;
    }
    for(int i = 0; i < batches.length(); i++)
    {
        modelbatch &b = batches[i];
        if(!(b.flags&Model_Mapmodel) || !b.m->animated())
        {
            continue;
        }
        for(int j = b.batched; j >= 0;)
        {
            batchedmodel &bm = batchedmodels[j];
            j = bm.next;
            visible |= bm.visible;
        }
    }
    return visible;
}

int batcheddynamicmodelbounds(int mask, vec &bbmin, vec &bbmax)
{
    int vis = 0;
    for(uint i = 0; i < batchedmodels.size(); i++)
    {
        batchedmodel &b = batchedmodels[i];
        if(b.flags&Model_Mapmodel)
        {
            break;
        }
        if(b.visible&mask)
        {
            bbmin.min(vec(b.center).sub(b.radius));
            bbmax.max(vec(b.center).add(b.radius));
            ++vis;
        }
    }
    for(int i = 0; i < batches.length(); i++)
    {
        modelbatch &b = batches[i];
        if(!(b.flags&Model_Mapmodel) || !b.m->animated())
        {
            continue;
        }
        for(int j = b.batched; j >= 0;)
        {
            batchedmodel &bm = batchedmodels[j];
            j = bm.next;
            if(bm.visible&mask)
            {
                bbmin.min(vec(bm.center).sub(bm.radius));
                bbmax.max(vec(bm.center).add(bm.radius));
                ++vis;
            }
        }
    }
    return vis;
}

void rendershadowmodelbatches(bool dynmodel)
{
    for(int i = 0; i < batches.length(); i++)
    {
        modelbatch &b = batches[i];
        if(!b.m->shadow || (!dynmodel && (!(b.flags&Model_Mapmodel) || b.m->animated())))
        {
            continue;
        }
        bool rendered = false;
        for(int j = b.batched; j >= 0;)
        {
            batchedmodel &bm = batchedmodels[j];
            j = bm.next;
            if(!(bm.visible&(1<<shadowside)))
            {
                continue;
            }
            if(!rendered)
            {
                b.m->startrender();
                rendered = true;
            }
            renderbatchedmodel(b.m, bm);
        }
        if(rendered)
        {
            b.m->endrender();
        }
    }
}

void rendermapmodelbatches()
{
    enableaamask();
    for(int i = 0; i < batches.length(); i++)
    {
        modelbatch &b = batches[i];
        if(!(b.flags&Model_Mapmodel))
        {
            continue;
        }
        b.m->startrender();
        setaamask(b.m->animated());
        for(int j = b.batched; j >= 0;)
        {
            batchedmodel &bm = batchedmodels[j];
            renderbatchedmodel(b.m, bm);
            j = bm.next;
        }
        b.m->endrender();
    }
    disableaamask();
}

float transmdlsx1 = -1,
      transmdlsy1 = -1,
      transmdlsx2 = 1,
      transmdlsy2 = 1;
uint transmdltiles[lighttilemaxheight];

void rendermodelbatches()
{
    transmdlsx1 = transmdlsy1 = 1;
    transmdlsx2 = transmdlsy2 = -1;
    memset(transmdltiles, 0, sizeof(transmdltiles));

    enableaamask();
    for(int i = 0; i < batches.length(); i++)
    {
        modelbatch &b = batches[i];
        if(b.flags&Model_Mapmodel)
        {
            continue;
        }
        bool rendered = false;
        for(int j = b.batched; j >= 0;)
        {
            batchedmodel &bm = batchedmodels[j];
            j = bm.next;
            bm.culled = cullmodel(b.m, bm.center, bm.radius, bm.flags, bm.d);
            if(bm.culled || bm.flags&Model_OnlyShadow)
            {
                continue;
            }
            if(bm.colorscale.a < 1 || bm.flags&Model_ForceTransparent)
            {
                float sx1, sy1, sx2, sy2;
                ivec bbmin(vec(bm.center).sub(bm.radius)), bbmax(vec(bm.center).add(bm.radius+1));
                if(calcbbscissor(bbmin, bbmax, sx1, sy1, sx2, sy2))
                {
                    transmdlsx1 = min(transmdlsx1, sx1);
                    transmdlsy1 = min(transmdlsy1, sy1);
                    transmdlsx2 = max(transmdlsx2, sx2);
                    transmdlsy2 = max(transmdlsy2, sy2);
                    masktiles(transmdltiles, sx1, sy1, sx2, sy2);
                }
                continue;
            }
            if(!rendered)
            {
                b.m->startrender();
                rendered = true;
                setaamask(true);
            }
            if(bm.flags&Model_CullQuery)
            {
                bm.d->query = newquery(bm.d);
                if(bm.d->query)
                {
                    startquery(bm.d->query);
                    renderbatchedmodel(b.m, bm);
                    endquery();
                    continue;
                }
            }
            renderbatchedmodel(b.m, bm);
        }
        if(rendered)
        {
            b.m->endrender();
        }
        if(b.flags&Model_CullQuery)
        {
            bool queried = false;
            for(int j = b.batched; j >= 0;)
            {
                batchedmodel &bm = batchedmodels[j];
                j = bm.next;
                if(bm.culled&(Model_CullOccluded|Model_CullQuery) && bm.flags&Model_CullQuery)
                {
                    if(!queried)
                    {
                        if(rendered)
                        {
                            setaamask(false);
                        }
                        enablecullmodelquery();
                        queried = true;
                    }
                    rendercullmodelquery(b.m, bm.d, bm.center, bm.radius);
                }
            }
            if(queried)
            {
                disablecullmodelquery();
            }
        }
    }
    disableaamask();
}

void rendertransparentmodelbatches(int stencil)
{
    enableaamask(stencil);
    for(int i = 0; i < batches.length(); i++)
    {
        modelbatch &b = batches[i];
        if(b.flags&Model_Mapmodel)
        {
            continue;
        }
        bool rendered = false;
        for(int j = b.batched; j >= 0;)
        {
            batchedmodel &bm = batchedmodels[j];
            j = bm.next;
            bm.culled = cullmodel(b.m, bm.center, bm.radius, bm.flags, bm.d);
            if(bm.culled || !(bm.colorscale.a < 1 || bm.flags&Model_ForceTransparent) || bm.flags&Model_OnlyShadow)
            {
                continue;
            }
            if(!rendered)
            {
                b.m->startrender();
                rendered = true;
                setaamask(true);
            }
            if(bm.flags&Model_CullQuery)
            {
                bm.d->query = newquery(bm.d);
                if(bm.d->query)
                {
                    startquery(bm.d->query);
                    renderbatchedmodel(b.m, bm);
                    endquery();
                    continue;
                }
            }
            renderbatchedmodel(b.m, bm);
        }
        if(rendered)
        {
            b.m->endrender();
        }
    }
    disableaamask();
}

static occludequery *modelquery = NULL;
static int modelquerybatches = -1,
           modelquerymodels = -1,
           modelqueryattached = -1;

void startmodelquery(occludequery *query)
{
    modelquery = query;
    modelquerybatches = batches.length();
    modelquerymodels = batchedmodels.size();
    modelqueryattached = modelattached.length();
}

void endmodelquery()
{
    if(static_cast<int>(batchedmodels.size()) == modelquerymodels)
    {
        modelquery->fragments = 0;
        modelquery = NULL;
        return;
    }
    enableaamask();
    startquery(modelquery);
    for(int i = 0; i < batches.length(); i++)
    {
        modelbatch &b = batches[i];
        int j = b.batched;
        if(j < modelquerymodels)
        {
            continue;
        }
        b.m->startrender();
        setaamask(!(b.flags&Model_Mapmodel) || b.m->animated());
        do
        {
            batchedmodel &bm = batchedmodels[j];
            renderbatchedmodel(b.m, bm);
            j = bm.next;
        } while(j >= modelquerymodels);
        b.batched = j;
        b.m->endrender();
    }
    endquery();
    modelquery = NULL;
    batches.setsize(modelquerybatches);
    batchedmodels.resize(modelquerymodels);
    modelattached.setsize(modelqueryattached);
    disableaamask();
}

void clearbatchedmapmodels()
{
    for(int i = 0; i < batches.length(); i++)
    {
        modelbatch &b = batches[i];
        if(b.flags&Model_Mapmodel)
        {
            batchedmodels.resize(b.batched);
            batches.setsize(i);
            break;
        }
    }
}

void rendermapmodel(int idx, int anim, const vec &o, float yaw, float pitch, float roll, int flags, int basetime, float size)
{
    if(!mapmodels.inrange(idx))
    {
        return;
    }
    mapmodelinfo &mmi = mapmodels[idx];
    model *m = mmi.m ? mmi.m : loadmodel(mmi.name);
    if(!m)
    {
        return;
    }
    vec center, bbradius;
    m->boundbox(center, bbradius);
    float radius = bbradius.magnitude();
    center.mul(size);
    if(roll)
    {
        center.rotate_around_y(-roll*RAD);
    }
    if(pitch && m->pitched())
    {
        center.rotate_around_x(pitch*RAD);
    }
    center.rotate_around_z(yaw*RAD);
    center.add(o);
    radius *= size;

    int visible = 0;
    if(shadowmapping)
    {
        if(!m->shadow)
        {
            return;
        }
        visible = shadowmaskmodel(center, radius);
        if(!visible)
        {
            return;
        }
    }
    else if(flags&(Model_CullVFC|Model_CullDist|Model_CullOccluded) && cullmodel(m, center, radius, flags))
    {
        return;
    }
    batchedmodel b = batchedmodel();
    b.pos = o;
    b.center = center;
    b.radius = radius;
    b.anim = anim;
    b.yaw = yaw;
    b.pitch = pitch;
    b.roll = roll;
    b.basetime = basetime;
    b.basetime2 = 0;
    b.sizescale = size;
    b.colorscale = vec4(1, 1, 1, 1);
    b.flags = flags | Model_Mapmodel;
    b.visible = visible;
    b.d = NULL;
    b.attached = -1;
    addbatchedmodel(m, b, batchedmodels.size()-1);
    batchedmodels.push_back(b);
}

void rendermodel(const char *mdl, int anim, const vec &o, float yaw, float pitch, float roll, int flags, dynent *d, modelattach *a, int basetime, int basetime2, float size, const vec4 &color)
{
    model *m = loadmodel(mdl);
    if(!m)
    {
        return;
    }

    vec center, bbradius;
    m->boundbox(center, bbradius);
    float radius = bbradius.magnitude();
    if(d)
    {
        if(d->ragdoll)
        {
            if(anim & Anim_Ragdoll && d->ragdoll->millis >= basetime)
            {
                radius = max(radius, d->ragdoll->radius);
                center = d->ragdoll->center;
                goto hasboundbox; //skip roll and pitch stuff
            }
            DELETEP(d->ragdoll);
        }
        if(anim & Anim_Ragdoll)
        {
            flags &= ~(Model_CullVFC | Model_CullOccluded | Model_CullQuery);
        }
    }
    center.mul(size);
    if(roll)
    {
        center.rotate_around_y(-roll*RAD);
    }
    if(pitch && m->pitched())
    {
        center.rotate_around_x(pitch*RAD);
    }
    center.rotate_around_z(yaw*RAD);
    center.add(o);
hasboundbox:
    radius *= size;

    if(flags&Model_NoRender)
    {
        anim |= Anim_NoRender;
    }

    if(a)
    {
        for(int i = 0; a[i].tag; i++)
        {
            if(a[i].name)
            {
                a[i].m = loadmodel(a[i].name);
            }
        }
    }

    if(flags&Model_CullQuery)
    {
        if(!oqfrags || !oqdynent || !d)
        {
            flags &= ~Model_CullQuery;
        }
    }

    if(flags&Model_NoBatch)
    {
        int culled = cullmodel(m, center, radius, flags, d);
        if(culled)
        {
            if(culled&(Model_CullOccluded|Model_CullQuery) && flags&Model_CullQuery)
            {
                enablecullmodelquery();
                rendercullmodelquery(m, d, center, radius);
                disablecullmodelquery();
            }
            return;
        }
        enableaamask();
        if(flags&Model_CullQuery)
        {
            d->query = newquery(d);
            if(d->query)
            {
                startquery(d->query);
            }
        }
        m->startrender();
        setaamask(true);
        if(flags&Model_FullBright)
        {
            anim |= Anim_FullBright;
        }
        m->render(anim, basetime, basetime2, o, yaw, pitch, roll, d, a, size, color);
        m->endrender();
        if(flags&Model_CullQuery && d->query)
        {
            endquery();
        }
        disableaamask();
        return;
    }

    batchedmodel b = batchedmodel();
    b.pos = o;
    b.center = center;
    b.radius = radius;
    b.anim = anim;
    b.yaw = yaw;
    b.pitch = pitch;
    b.roll = roll;
    b.basetime = basetime;
    b.basetime2 = basetime2;
    b.sizescale = size;
    b.colorscale = color;
    b.flags = flags;
    b.visible = 0;
    b.d = d;
    b.attached = a ? modelattached.length() : -1;
    if(a)
    {
        for(int i = 0;; i++)
        {
            modelattached.add(a[i]);
            if(!a[i].tag)
            {
                break;
            }
        }
    }
    addbatchedmodel(m, b, batchedmodels.size()-1);
    batchedmodels.push_back(b);
}

int intersectmodel(const char *mdl, int anim, const vec &pos, float yaw, float pitch, float roll, const vec &o, const vec &ray, float &dist, int mode, dynent *d, modelattach *a, int basetime, int basetime2, float size)
{
    model *m = loadmodel(mdl);
    if(!m)
    {
        return -1;
    }
    if(d && d->ragdoll && (!(anim & Anim_Ragdoll) || d->ragdoll->millis < basetime))
    {
        DELETEP(d->ragdoll);
    }
    if(a)
    {
        for(int i = 0; a[i].tag; i++)
        {
            if(a[i].name)
            {
                a[i].m = loadmodel(a[i].name);
            }
        }
    }
    return m->intersect(anim, basetime, basetime2, pos, yaw, pitch, roll, d, a, size, o, ray, dist, mode);
}

void abovemodel(vec &o, const char *mdl)
{
    model *m = loadmodel(mdl);
    if(!m)
    {
        return;
    }
    o.z += m->above();
}

std::vector<int> findanims(const char *pattern)
{
    std::vector<int> anims;
    for(int i = 0; i < static_cast<int>(animnames.size()); ++i)
    {
        if(!animnames.at(i).compare(pattern))
        {
            anims.push_back(i);
        }
    }
    return anims;
}

ICOMMAND(findanims, "s", (char *name),
{
    std::vector<int> anims = findanims(name);
    vector<char> buf;
    string num;
    for(int i = 0; i < static_cast<int>(anims.size()); i++)
    {
        formatstring(num, "%d", anims[i]);
        if(i > 0)
        {
            buf.add(' ');
        }
        buf.put(num, strlen(num));
    }
    buf.add('\0');
    result(buf.getbuf());
});

#define TRY_LOAD(tex, prefix, cmd, name) \
    if((tex = textureload(makerelpath(mdir, name ".jpg", prefix, cmd), 0, true, false))==notexture) \
    { \
        if((tex = textureload(makerelpath(mdir, name ".png", prefix, cmd), 0, true, false))==notexture) \
        { \
            if((tex = textureload(makerelpath(mdir, name ".jpg", prefix, cmd), 0, true, false))==notexture) \
            { \
                if((tex = textureload(makerelpath(mdir, name ".png", prefix, cmd), 0, true, false))==notexture) \
                { \
                    return; \
                } \
            } \
        } \
    }

void loadskin(const char *dir, const char *altdir, Texture *&skin, Texture *&masks) // model skin sharing
{
    DEF_FORMAT_STRING(mdir, "media/model/%s", dir);
    DEF_FORMAT_STRING(maltdir, "media/model/%s", altdir);
    masks = notexture;
    TRY_LOAD(skin, NULL, NULL, "skin");
    TRY_LOAD(masks, NULL, NULL, "masks");
}

#undef TRY_LOAD

void setbbfrommodel(dynent *d, const char *mdl)
{
    model *m = loadmodel(mdl);
    if(!m)
    {
        return;
    }
    vec center, radius;
    m->collisionbox(center, radius);
    if(m->collide != Collide_Ellipse)
    {
        d->collidetype = Collide_OrientedBoundingBox;
    }
    d->xradius   = radius.x + fabs(center.x);
    d->yradius   = radius.y + fabs(center.y);
    d->radius    = d->collidetype==Collide_OrientedBoundingBox ? sqrtf(d->xradius*d->xradius + d->yradius*d->yradius) : max(d->xradius, d->yradius);
    d->eyeheight = (center.z-radius.z) + radius.z*2*m->eyeheight;
    d->aboveeye  = radius.z*2*(1.0f-m->eyeheight);
    if (d->aboveeye + d->eyeheight <= 0.5f)
    {
        float zrad = (0.5f - (d->aboveeye + d->eyeheight)) / 2;
        d->aboveeye += zrad;
        d->eyeheight += zrad;
    }
}

