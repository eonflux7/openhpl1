#include <algorithm>
#include <array>
#include <clocale>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif
#include <glad/gl.h>

// Inspect batch contents without a graphics context, as with the cache fixtures below.
#define private public
#include "graphics/SpriteBatch.h"
#undef private
#include "impl/LowLevelGraphicsSDL.h"
#include "graphics/GraphicsDrawer.h"
#include "gui/GuiSet.h"
#include "gui/GuiMaterialBasicTypes.h"

#include "math/Math.h"
#include "math/PidController.h"
#include "graphics/VertexBuffer.h"
#include "graphics/RenderList.h"
#include "graphics/GPUProgram.h"
#include "graphics/Animation.h"
#include "graphics/AnimationTrack.h"
#include "graphics/Mesh.h"
#include "graphics/Skeleton.h"
#include "graphics/Bone.h"
#include "impl/Platform.h"
#include "resources/MeshManager.h"
#include "system/String.h"
#include "tinyxml.h"
#include "impl/LowLevelPhysicsNewton.h"
#include "impl/PhysicsWorldNewton.h"
#include "physics/CollideData.h"
#include "physics/PhysicsBody.h"
#include "physics/PhysicsJointHinge.h"
#include "physics/PhysicsJointSlider.h"
#include "physics/PhysicsJointScrew.h"

// Exercise search/cache internals without a physics world or graphics context.
// These tests still call the actual engine methods, including CPU skinning.
#define private public
#define protected public
#include "ai/AStar.h"
#include "ai/AINodeContainer.h"
#include "graphics/SubMesh.h"
#include "graphics/Graphics.h"
#include "graphics/ParticleEmitter3D.h"
#include "graphics/ParticleSystem3D.h"
#include "graphics/BillBoard.h"
#include "graphics/Beam.h"
#include "scene/Camera3D.h"
#include "impl/MeshLoaderCollada.h"
#include "impl/VertexBufferVBO.h"
#include "impl/LowLevelSoundOpenAL.h"
#include "impl/LowLevelResourcesSDL.h"
#include "resources/Resources.h"
#include "resources/TextureManager.h"
#include "scene/Light3DPoint.h"
#include "scene/AnimationState.h"
#include "scene/MeshEntity.h"
#include "scene/SubMeshEntity.h"
#include "scene/World3D.h"
#include "sound/SoundHandler.h"
#include "sound/MusicHandler.h"
#include "sound/SoundChannel.h"
#undef protected
#undef private

using namespace hpl;

static void Check(bool result, const char *message)
{
	if(!result) throw std::runtime_error(message);
}

static bool Near(float a, float b) { return std::fabs(a - b) < 0.0001f; }

class TestVertexBuffer : public iVertexBuffer
{
public:
	explicit TestVertexBuffer(int count) : iVertexBuffer(nullptr, 0, eVertexBufferDrawType_Tri,
		eVertexBufferUsageType_Static, 0, 0), positions(count * 4, 1), normals(count * 3, 0), tangents(count * 4, 0) {}
	std::vector<float> positions, normals, tangents;
	void AddVertex(tVertexFlag, const cVector3f&) override {}
	void AddColor(tVertexFlag, const cColor&) override {}
	void AddIndex(unsigned) override {}
	bool Compile(tVertexCompileFlag) override { return true; }
	void UpdateData(tVertexFlag, bool) override {}
	void Transform(const cMatrixf&) override {}
	void Draw(eVertexBufferDrawType) override {}
	void DrawIndices(unsigned*, int, eVertexBufferDrawType) override {}
	void Bind() override {}
	void UnBind() override {}
	iVertexBuffer* CreateCopy(eVertexBufferUsageType) override { return new TestVertexBuffer(*this); }
	cBoundingVolume CreateBoundingVolume() override { return cBoundingVolume(); }
	float* GetArray(tVertexFlag flag) override
	{
		if(flag == eVertexFlag_Position) return positions.data();
		if(flag == eVertexFlag_Normal) return normals.data();
		if(flag == eVertexFlag_Tangent) return tangents.data();
		throw std::runtime_error("unexpected vertex array");
	}
	unsigned* GetIndices() override { return nullptr; }
	int GetVertexNum() override { return positions.size() / 4; }
	int GetIndexNum() override { return 0; }
	void ResizeArray(tVertexFlag, int) override {}
	void ResizeIndices(int) override {}
	cVector3f GetVector3(tVertexFlag, unsigned) override { return cVector3f(0); }
	cVector3f GetVector4(tVertexFlag, unsigned) override { return cVector3f(0); }
	cColor GetColor(tVertexFlag, unsigned) override { return cColor(1); }
	unsigned GetIndex(tVertexFlag, unsigned) override { return 0; }
};

// Resources normally initializes these during Init(). This fixture supplies just
// the owner used by a mesh entity, without starting the graphics/audio modules.
class TestResources : public cResources
{
public:
	TestResources() : cResources(nullptr, nullptr)
	{
		mpLowLevelSystem = nullptr;
		mpImageManager = nullptr; mpGpuProgramManager = nullptr;
		mpParticleManager = nullptr; mpSoundManager = nullptr;
		mpFontManager = nullptr; mpScriptManager = nullptr; mpTextureManager = nullptr;
		mpMaterialManager = nullptr; mpSoundEntityManager = nullptr; mpAnimationManager = nullptr;
		mpVideoManager = nullptr; mpMeshManager = nullptr; mpMeshLoaderHandler = nullptr;
	}
};

// Exercise the actual effect builders without a context. Only GPU allocation
// is stubbed; facing matrices, line fallback, local transforms and UV selection
// run through their production methods.
class TestEmitter : public iParticleEmitter3D
{
public:
	TestEmitter(cGraphics *graphics, cResources *resources, tMaterialVec *materials)
		: iParticleEmitter3D("fixture",materials,2,cVector3f(1),graphics,resources)
	{
		mPEType = ePEType_Normal; mbUsePartSpin = true; mpWorld = nullptr;
		mvDrawSize = cVector2f(0.2f,0.4f); mvMaxDrawSize = 1;
		mlNumOfParticles = 2;
		for(unsigned i=0; i<2; ++i)
		{
			*mvParticles[i] = cParticle{};
			mvParticles[i]->mvPos = cVector3f(1+i,2,-4);
			mvParticles[i]->mvLastPos = cVector3f(0,1,-5);
			mvParticles[i]->mvSize = cVector2f(2,3);
			mvParticles[i]->mfSpin = 0.7f;
			mvParticles[i]->mColor = cColor(0.2f,0.4f,0.6f,0.3f);
			mvParticles[i]->mlSubDivNum = i+1;
		}
		SetSubDivUV(cVector2l(2,2));
	}
	void UpdateMotion(float) override {}
	void SetParticleDefaults(cParticle*) override {}
};
static void CheckVector(cVector3f actual, cVector3f expected, const char *message)
{
	Check(Near(actual.x,expected.x) && Near(actual.y,expected.y) && Near(actual.z,expected.z), message);
}
static void TestEffectInstances()
{
	auto gen=glad_glGenBuffers; auto del=glad_glDeleteBuffers;
	glad_glGenBuffers=[](GLsizei n,GLuint* out){static GLuint next=1; while(n--) *out++=next++;};
	glad_glDeleteBuffers=[](GLsizei,const GLuint*){};
	{
		cLowLevelGraphicsSDL low;
		cGraphics graphics(&low,nullptr); graphics.mpRenderList=nullptr;
		TestResources resources;
		cCamera3D camera; camera.SetPosition(cVector3f(0,0,3));
		cParticleSystem3D parent("parent",nullptr,&resources,&graphics);
		parent.SetMatrix(cMath::MatrixMul(cMath::MatrixTranslate(cVector3f(2,3,4)), cMath::MatrixRotateY(0.4f)));
		tMaterialVec materials;
		TestEmitter emitter(&graphics,&resources,&materials); emitter.SetSystem(&parent);
		emitter.SetMatrix(cMath::MatrixRotateZ(0.3f));
		for(int local=0; local<2; ++local)
		for(int mode=0; mode<eParticleEmitter3DType_LastEnum; ++mode)
		for(int spin=0; spin<2; ++spin)
		{
			emitter.mCoordSystem=(eParticleEmitter3DCoordSystem)local;
			emitter.mDrawType=(eParticleEmitter3DType)mode;
			emitter.mbUsePartSpin=spin;
			emitter.mbUpdateGfx=true;
			emitter.UpdateGraphics(&camera,0,nullptr);
			const auto &instances=emitter.GetVertexBuffer()->GetQuadInstances();
			Check(instances.size()==2 && emitter.GetVertexBuffer()->GetIndexNum()==12,"one compact record per live particle");
			for(unsigned i=0;i<2;++i)
			{
				const cParticle &particle=*emitter.mvParticles[i];
				cVector3f position=particle.mvPos, last=particle.mvLastPos;
				if(local){position=cMath::MatrixMul(parent.GetWorldMatrix(),position); last=cMath::MatrixMul(parent.GetWorldMatrix(),last);}
				if(mode!=eParticleEmitter3DType_Axis){position=cMath::MatrixMul(camera.GetViewMatrix(),position);last=cMath::MatrixMul(camera.GetViewMatrix(),last);}
				const cMatrixf axis=cMath::MatrixInverse(emitter.GetWorldMatrix());
				for(int corner=0;corner<4;++corner)
				{
					const float x=corner==0||corner==3?1:-1, y=corner<2?-1:1;
					cVector3f expected;
					if(mode==eParticleEmitter3DType_Axis)
						expected=position+(axis.GetRight()*(x*particle.mvSize.x)+axis.GetForward()*(-y*particle.mvSize.y));
					else if(mode==eParticleEmitter3DType_Line)
					{
						cVector3f along=position-last; along.Normalise();
						cVector2f across(along.y,-along.x); across.Normalise();
						expected=(corner<2?last:position)+along*(y*emitter.mvDrawSize.y*particle.mvSize.y)+
							cVector3f(across.x,across.y,0)*(x*emitter.mvDrawSize.x*particle.mvSize.x);
					}
					else
					{
						cVector3f add(x*emitter.mvDrawSize.x,y*emitter.mvDrawSize.y,0);
						if(mode==eParticleEmitter3DType_DynamicPoint){add=add*cVector3f(particle.mvSize);if(spin)add=cMath::MatrixMul(cMath::MatrixRotateZ(particle.mfSpin),add);}
						expected=position+add;
					}
					CheckVector(emitter.GetVertexBuffer()->GetVector3(eVertexFlag_Position,i*4+corner),expected,"compact particle matches previous corner formula in all modes");
				}
				Check(instances[i].mColor==particle.mColor && instances[i].mEndColor==particle.mColor,"particle colour retained");
				Check(Near(instances[i].mvUV[0],i==0?0.5f:0) && Near(instances[i].mvUV[1],i==0?0.5f:1),"subdivision row/column order");
				Check(particle.mvPos==cVector3f(1+i,2,-4) && Near(particle.mfSpin,0.7f),"render preparation does not mutate simulation");
			}
		}
		emitter.mCoordSystem=eParticleEmitter3DCoordSystem_World;
		emitter.mDrawType=eParticleEmitter3DType_Line;
		emitter.mvParticles[0]->mvLastPos=emitter.mvParticles[0]->mvPos;
		emitter.mbUpdateGfx=true;emitter.UpdateGraphics(&camera,0,nullptr);
		CheckVector(emitter.GetVertexBuffer()->GetQuadInstances()[0].mvRight,cVector3f(0.4f,0,0),"stationary line width fallback");
		CheckVector(emitter.GetVertexBuffer()->GetQuadInstances()[0].mvUp,cVector3f(0,1.2f,0),"stationary line length fallback");
		emitter.KillInstantly();emitter.mbUpdateGfx=true;emitter.UpdateGraphics(&camera,0,nullptr);
		Check(emitter.GetVertexBuffer()->GetQuadInstances().empty(),"dead emitter submits no stale particles");

		cBillboard billboard("billboard",cVector2f(2,4),&resources,&graphics);
		billboard.SetSize(cVector2f(4,6)); billboard.SetColor(cColor(0.2f,0.3f,0.4f,0.5f));
		const auto &sheet=billboard.GetVertexBuffer()->GetQuadInstances()[0];
		CheckVector(sheet.mvRight,cVector3f(2,0,0),"billboard half width");
		CheckVector(sheet.mvUp,cVector3f(0,3,0),"billboard half height");
		Check(sheet.mvUV[1]==0 && sheet.mvUV[3]==1 && sheet.mColor==billboard.GetColor(),"billboard UV orientation and colour");
		billboard.SetPosition(cVector3f(1,2,-4)); billboard.SetForwardOffset(0.3f);
		for(auto type:{eBillboardType_Point,eBillboardType_Axis})
		{
			billboard.SetType(type);billboard.SetAxis(cVector3f(0.2f,1,0));
			const cMatrixf matrix=*billboard.GetModelMatrix(&camera);
			cVector3f toward=camera.GetPosition()-billboard.GetWorldPosition();toward.Normalise();
			CheckVector(matrix.GetTranslation(),billboard.GetWorldPosition()+toward*0.3f,"billboard forward offset retained");
			cVector3f expectedRight=cMath::Vector3Cross(type==eBillboardType_Point?camera.GetViewMatrix().GetUp():billboard.GetAxis(),toward);
			if(type==eBillboardType_Axis) expectedRight.Normalise();
			CheckVector(cVector3f(matrix.m[0][0],matrix.m[1][0],matrix.m[2][0]),expectedRight,"billboard facing retains original point/axis alignment");
		}
		Check(!billboard.IsShadowCaster(),"billboard remains excluded from shadows");

		cBeam beam("beam",&resources,&graphics);
		beam.SetPosition(cVector3f(1,2,-4)); beam.GetEnd()->SetPosition(cVector3f(1,7,-4));
		beam.SetSize(cVector2f(2,0.5f));beam.SetMultiplyAlphaWithColor(true);
		beam.SetColor(cColor(0.8f,0.4f,0.2f,0.5f));beam.GetEnd()->SetColor(cColor(0.4f,0.8f,0.2f,0.25f));
		beam.UpdateGraphics(&camera,0,nullptr);
		const auto tiled=beam.GetVertexBuffer()->GetQuadInstances()[0];
		CheckVector(tiled.mvUp,cVector3f(0,2.5f,0),"beam endpoint distance");
		Check(tiled.mvUV[1]==1 && tiled.mvUV[3]==-10,"beam tiling retains negative terminal coordinate");
		Check(Near(tiled.mColor.r,0.4f) && Near(tiled.mEndColor.g,0.2f),"beam endpoint alpha multiplication");
		CheckVector(beam.GetModelMatrix(&camera)->GetTranslation(),cVector3f(1,4.5f,-4),"beam midpoint facing");
		beam.SetMultiplyAlphaWithColor(false);beam.UpdateGraphics(&camera,0,nullptr);
		Check(beam.GetVertexBuffer()->GetQuadInstances()[0].mColor==tiled.mColor &&
			beam.GetVertexBuffer()->GetQuadInstances()[0].mEndColor==tiled.mEndColor,"beam flag retains colours until their setters run");
		beam.SetColor(cColor(0.7f,0.4f,0.2f,0.5f));
		Check(Near(beam.GetVertexBuffer()->GetQuadInstances()[0].mColor.r,0.7f) &&
			beam.GetVertexBuffer()->GetQuadInstances()[0].mEndColor==tiled.mEndColor,"beam setters update each endpoint independently");
		beam.SetTileHeight(false);beam.UpdateGraphics(&camera,0,nullptr);
		Check(beam.GetVertexBuffer()->GetQuadInstances()[0].mvUV[3]==0,"untiled beam UVs");
		beam.GetEnd()->SetPosition(cVector3f(4,3,-2));beam.UpdateGraphics(&camera,0,nullptr);
		CheckVector(beam.GetMidPosition(),cVector3f(2.5f,2.5f,-3),"moving beam endpoint updates instance");
		Check(!beam.IsShadowCaster(),"beam remains excluded from shadows");
	}
	glad_glGenBuffers=gen;glad_glDeleteBuffers=del;
}

static void TestSkinning()
{
	cSubMesh tinyWeights("tiny", nullptr);
	tinyWeights.SetVertexBuffer(new TestVertexBuffer(1));
	tinyWeights.AddVertexBonePair(cVertexBonePair(0, 0, std::numeric_limits<float>::denorm_min()));
	tinyWeights.AddVertexBonePair(cVertexBonePair(0, 1, std::numeric_limits<float>::max()));
	tinyWeights.CompileBonePairs();
	Check(tinyWeights.mpVertexWeights[0] == 1 && tinyWeights.mpVertexBones[0] == 1, "normalized weights stay packed after underflow");
	TestResources resources;
	cMeshManager manager(nullptr, &resources);
	auto *mesh = new cMesh("skin", nullptr, nullptr);
	mesh->SetSkeleton(new cSkeleton());
	for(int i = 0; i < 4; ++i) mesh->GetSkeleton()->GetRootBone()->CreateChildBone(std::to_string(i));
	auto *sub = mesh->CreateSubMesh("vertices");
	auto *buffer = new TestVertexBuffer(3);
	buffer->positions = {0,0,0,1, 5,2,3,1, 9,0,0,1};
	buffer->normals = {0,0,1, 0,1,0, 0,0,1};
	buffer->tangents = {1,0,0,1, 0,0,1,-1, 1,0,0,1};
	sub->SetVertexBuffer(buffer);
	sub->AddVertexBonePair(cVertexBonePair(0, 0, 0.2f));
	sub->AddVertexBonePair(cVertexBonePair(0, 1, 0.2f));
	for(int i = 0; i < 4; ++i) sub->AddVertexBonePair(cVertexBonePair(2, i, (i + 1) * 0.1f));
	sub->AddVertexBonePair(cVertexBonePair(99, 0, 1));
	sub->AddVertexBonePair(cVertexBonePair(1, -1, 1));
	sub->AddVertexBonePair(cVertexBonePair(1, 256, 1));
	sub->AddVertexBonePair(cVertexBonePair(1, 0, std::numeric_limits<float>::quiet_NaN()));
	sub->CompileBonePairs();
	Check(Near(sub->mpVertexWeights[0], 0.5f) && Near(sub->mpVertexWeights[1], 0.5f), "bone weight normalization");
	sub->CompileBonePairs(); // Recompilation must release the old arrays too.
	cMeshEntity entity("skin", mesh, nullptr, &manager, nullptr);
	for(int i = 0; i < 4; ++i)
	{
		entity.mvBoneMatrices[i] = cMatrixf::Identity;
		entity.mvBoneMatrices[i].SetTranslation(cVector3f((i + 1) * 10, 0, 0));
	}
	auto *subEntity = entity.GetSubMeshEntity(0);
	subEntity->UpdateGraphics(nullptr, 0, nullptr);
	float *skinned = subEntity->mpDynVtxBuffer->GetArray(eVertexFlag_Position);
	Check(Near(skinned[0], 15), "weighted vertex position");
	Check(std::equal(skinned + 4, skinned + 8, buffer->positions.begin() + 4), "unweighted bind pose");
	Check(Near(skinned[8], 39), "vertex after an unweighted vertex, with four influences");
	Check(std::equal(subEntity->mpDynVtxBuffer->GetArray(eVertexFlag_Normal) + 3,
		subEntity->mpDynVtxBuffer->GetArray(eVertexFlag_Normal) + 6, buffer->normals.begin() + 3), "unweighted normal");
	Check(subEntity->mpDynVtxBuffer->GetArray(eVertexFlag_Tangent)[7] == -1, "unweighted tangent handedness");
}

static void TestStrings()
{
	float values[3]; unsigned ints[3];
	Check(!cString::FloatStringToArray(values, "1 2", 3), "truncated floats");
	Check(!cString::UIntStringToArray(ints, "1 2", 3), "truncated integers");
	Check(cString::FloatStringToArray(values, "1\n2\t3\r ", 3) && values[2] == 3, "float whitespace");
	Check(cString::UIntStringToArray(ints, "1\n2\t3", 3) && ints[2] == 3, "integer whitespace");
	Check(cString::FloatStringToArray(values, "0.12345678901234567890123456789", 1), "long float token");
	for(const char *bad : {"nan", "inf", "1e100", "1x", "1 2"})
		Check(!cString::FloatStringToArray(values, bad, 1), "invalid float token/count");
	for(const char *bad : {"-1", "4294967296", "99999999999999999999999999999", "1.0", "1x"})
		Check(!cString::UIntStringToArray(ints, bad, 1), "invalid integer token/range");
	Check(cString::FloatStringToArray(nullptr, " \n", 0), "empty array");
	Check(!cString::FloatStringToArray(values, nullptr, 1), "null input");
	Check(!cString::FloatStringToArray(values, "1", -1), "negative count");
	std::setlocale(LC_CTYPE, "C");
	Check(cString::To16Char("\xc3\xa9").empty(), "invalid multibyte conversion");
	Check(cString::To8Char(L"\u00e9").empty(), "unrepresentable wide conversion");
	Check(cString::To16Char("abc") == L"abc" && cString::To8Char(L"abc") == "abc", "ASCII conversion");
}

static void TestPathfinding(const std::filesystem::path &directory)
{
	cAINodeContainer graph("test", "nodes", nullptr, cVector3f(1));
	graph.BuildNodeGridMap();
	Check(!graph.GetNodeIterator(cVector3f(0), 1).HasNext(), "empty grid");
	for(const char *name : {"start", "detour", "shortcut", "goal"}) graph.AddNode(name, cVector3f(0));
	graph.BuildNodeGridMap(); graph.BuildNodeGridMap();
	auto it = graph.GetNodeIterator(cVector3f(0), 1);
	int count = 0;
	while(it.HasNext()) { it.Next(); ++count; }
	Check(count == 4, "collapsed grid dimensions/repeated grid build");
	auto edge = [&](int from, int to, float distance) {
		cAINodeEdge connection; connection.mpNode = graph.GetNode(to);
		connection.mfDistance = distance; connection.mfSqrDistance = distance * distance;
		graph.GetNode(from)->mvEdges.push_back(connection);
	};
	edge(0,1,10); edge(0,2,1); edge(2,1,1); edge(1,3,1); edge(2,3,20);
	cAStarHandler search(&graph);
	search.mvGoal = cVector3f(0);
	search.m_setGoalNodes.insert(graph.GetNode(3));
	search.AddOpenNode(graph.GetNode(0), nullptr, 0);
	search.IterateAlgorithm();
	Check(search.mpGoalNode && search.mpGoalNode->mfDistance == 3, "A* cheaper route to an open node");
	Check(search.mpGoalNode->mpParent->mpParent->mpAINode == graph.GetNode(2), "A* updated parent chain");
	class BlockShortcut : public iAStarCallback
	{
	public:
		bool CanAddNode(cAINode*, cAINode *child) override { return child->GetName() != "shortcut"; }
	} callback;
	cAStarHandler blocked(&graph);
	blocked.mvGoal = 0; blocked.SetCallback(&callback);
	blocked.m_setGoalNodes.insert(graph.GetNode(3));
	blocked.AddOpenNode(graph.GetNode(0), nullptr, 0); blocked.IterateAlgorithm();
	Check(blocked.mpGoalNode && blocked.mpGoalNode->mfDistance == 11, "A* callback rejects shortcut");
	cAStarHandler limited(&graph);
	limited.SetMaxIterations(1); limited.m_setGoalNodes.insert(graph.GetNode(3));
	limited.AddOpenNode(graph.GetNode(0), nullptr, 0); limited.IterateAlgorithm();
	Check(!limited.mpGoalNode, "A* iteration limit");
	// Height penalties apply only to the edge being added.
	graph.GetNode(1)->mvPosition.y = 1;
	cAStarHandler height(&graph);
	height.AddOpenNode(graph.GetNode(0), nullptr, 10);
	auto *parent = height.GetBestNode();
	height.AddOpenNode(graph.GetNode(1), parent, 1);
	Check(height.GetBestNode()->mfDistance == 12, "A* accumulated height penalty");
	graph.SaveToFile((directory / "nodes.xml").string());
	Check(graph.LoadFromFile((directory / "nodes.xml").string()), "AI cache round trip");
	Check(graph.LoadFromFile((directory / "nodes.xml").string()) && graph.GetNode(0)->GetEdgeNum() == 2, "AI cache replaces edges");
	std::ofstream(directory / "bad-nodes.xml") << "<AINodes><Node Name='missing'/></AINodes>";
	Check(!graph.LoadFromFile((directory / "bad-nodes.xml").string()) && graph.GetNode(0)->GetEdgeNum() == 2, "bad AI cache preserves graph");
}

static void TestTangents()
{
	const unsigned indices[] = {0,1,2};
	const float positions[] = {0,0,0, 1,0,0, 0,1,0};
	const float normals[] = {0,0,1, 0,0,1, 0,0,1};
	const float ordinary[] = {0,0,0, 1,0,0, 0,1,0};
	const float degenerate[] = {0,0,0, 0,0,0, 0,0,0};
	const float nearlyDegenerate[] = {0,0,0, 1,1,0, 2,2.0000002f,0};
	float tangent[12];
	for(auto uv : {ordinary, degenerate, nearlyDegenerate})
	{
		Check(cMath::CreateTriTangentVectors(tangent, indices, 3, positions, 3, uv, normals, 3), "tangent generation");
		for(int i = 0; i < 3; ++i)
		{
			Check(std::isfinite(tangent[i*4]) && std::isfinite(tangent[i*4+1]) && std::isfinite(tangent[i*4+2]), "finite degenerate tangents");
			Check(Near(tangent[i*4]*tangent[i*4] + tangent[i*4+1]*tangent[i*4+1], 1) && tangent[i*4+2] == 0, "unit orthogonal tangent");
		}
	}
	Check(!cMath::CreateTriTangentVectors(tangent, indices, 2, positions, 3, ordinary, normals, 3), "incomplete triangle");
}

static void TestAnimation(const std::filesystem::path &directory)
{
	cAnimation animation("test", "test"); animation.SetLength(1);
	auto *track = animation.CreateTrack("test", eAnimTransformFlag_Translate);
	Check(track->GetInterpolatedKeyFrame(0).scale == cVector3f(1), "empty animation track");
	for(float time : {0.0f, 0.5f, 1.0f})
	{
		auto *frame = track->CreateKeyFrame(time);
		frame->rotation = cQuaternion::Identity; frame->scale = 1; frame->trans = cVector3f(time,0,0);
	}
	Check(Near(track->GetInterpolatedKeyFrame(0.25f).trans.x, 0.25f), "keyframe interpolation");
	Check(track->GetInterpolatedKeyFrame(1).trans.x == 1, "last keyframe");
	cAnimationState state(&animation, "test", nullptr);
	state.SetLoop(true); state.SetTimePosition(0.9f); state.Update(0.2f);
	Check(state.HasEventInLastUpdate(0.95f) && state.HasEventInLastUpdate(0.05f), "events on both sides of a loop");
	Check(!state.HasEventInLastUpdate(0.5f), "event outside crossed interval");
	state.SetSpeed(-1); state.SetTimePosition(0.1f); state.Update(0.2f);
	Check(state.HasEventInLastUpdate(0.05f) && state.HasEventInLastUpdate(0.95f), "reverse loop events");
	state.SetTimePosition(1); state.Update(1);
	Check(state.HasEventInLastUpdate(0), "reverse full loop at exact endpoint");
	state.SetSpeed(1); state.Update(2);
	Check(state.HasEventInLastUpdate(0.5f), "multiple-loop update");
	state.SetPaused(true); state.Update(0.2f);
	Check(!state.HasEventInLastUpdate(0.5f), "paused event interval");
	state.SetBaseSpeed(0.75f);
	cSaveData_cAnimationState saved;
	state.SaveToSaveData(&saved);
	auto file = cString::To16Char((directory / "animation.xml").string());
	Check(cSerializeClass::SaveToFile(&saved, file, "Animation"), "animation serialization");
	cSaveData_cAnimationState restored;
	Check(cSerializeClass::LoadFromFile(&restored, file), "animation deserialization");
	Check(restored.mfBaseSpeed == 0.75f, "serialized base speed");
	state.LoadFromSaveData(&restored);
	Check(state.GetBaseSpeed() == 0.75f, "restored base speed");
	TiXmlDocument oldSave((directory / "animation.xml").string().c_str());
	Check(oldSave.LoadFile(), "read animation save");
	auto *savedClass = oldSave.RootElement()->FirstChildElement("class");
	for(auto *member = savedClass->FirstChildElement(); member; member = member->NextSiblingElement())
		if(cString::ToString(member->Attribute("name"), "") == "mfBaseSpeed")
		{
			savedClass->RemoveChild(member);
			break;
		}
	Check(oldSave.SaveFile(), "write legacy animation save");
	cSaveData_cAnimationState legacy;
	Check(cSerializeClass::LoadFromFile(&legacy, file) && legacy.mfBaseSpeed == 1, "legacy save base speed default");
}

static void TestDirectoryScan(const std::filesystem::path &directory)
{
	auto path = directory;
	for(int i = 0; i < 4; ++i) path /= std::string(64, 'a' + i);
	std::filesystem::create_directories(path);
	std::ofstream(path / "valid.txt") << "file";
	std::ofstream(path / std::string("invalid-\xff.txt")) << "file";
	tWStringList files;
	Platform::FindFileInDir(files, cString::To16Char(path.string()), L"*.txt");
	Check(files.size() == 1 && files.front() == L"valid.txt", "long directory path and invalid filename");
	Platform::FindFileInDir(files, L"", L"*");
	Check(files.size() == 1, "empty directory name");
}

static void TestColladaPolygons()
{
	cMeshLoaderCollada loader(nullptr);
	loader.mbZToY=false;
	const std::string sources = R"(<root><geometry id="quad"><mesh>
<source id="positions"><float_array>0 0 0 1 0 0 1 1 0 0 1 0</float_array><technique_common><accessor stride="3" count="4"/></technique_common></source>
<source id="normals"><float_array>0 0 1</float_array><technique_common><accessor stride="3" count="1"/></technique_common></source>
<source id="uv"><float_array>0 0 1 0 1 1 0 1</float_array><technique_common><accessor stride="2" count="4"/></technique_common></source>
<vertices id="vertices"><input semantic="POSITION" source="#positions"/></vertices>)";
	const std::string inputs = R"(<input semantic="VERTEX" source="#vertices" offset="0"/><input semantic="NORMAL" source="#normals" offset="1"/><input semantic="TEXCOORD" source="#uv" offset="2"/>)";
	for(const char* type : {"polylist", "polygons"}) {
		std::string xml = sources + "<" + type + " count=\"1\">" + inputs;
		if(std::string(type)=="polylist") xml += "<vcount>4</vcount>";
		xml += "<p>0 0 0 1 0 1 2 0 2 3 0 3</p></" + std::string(type) + "></mesh></geometry></root>";
		TiXmlDocument document;document.Parse(xml.c_str());
		tColladaGeometryVec geometry;cColladaScene scene;
		loader.LoadGeometry(document.RootElement(),geometry,&scene);
		Check(geometry.size()==1 && geometry[0].mvIndexVec.size()==6, "Collada quad becomes two triangles before caching");
		const int corners[]={0,1,2,0,2,3};
		const cVector3f positions[]={cVector3f(0,0,0),cVector3f(1,0,0),cVector3f(1,1,0),cVector3f(0,1,0)};
		for(int i=0; i<6; ++i) {
			const cVertex& vertex=geometry[0].mvVertexVec[geometry[0].mvIndexVec[i]];
			Check(vertex.pos==positions[corners[i]], "Collada triangle winding and shared corners");
			Check(vertex.tex.x==vertex.pos.x && vertex.tex.y==1-vertex.pos.y, "Collada corner UV indices retained");
		}
		Check(geometry[0].mvTangents.size()==geometry[0].mvVertexVec.size()*4, "triangulated geometry tangent layout");
	}
}

static void TestColladaCache(const std::filesystem::path &directory)
{
	cMeshLoaderCollada loader(nullptr);
	tColladaImageVec images; tColladaTextureVec textures; tColladaMaterialVec materials;
	tColladaLightVec lights; tColladaGeometryVec geometry; tColladaControllerVec controllers;
	tColladaAnimationVec animations; cColladaScene scene;
	scene.mfStartTime = scene.mfEndTime = 0;
	auto file = (directory / "cache.xml").string();
	auto save = [&] { return loader.SaveStructures(file, &images, &textures, &materials, &lights, &geometry, &controllers, &animations, &scene); };
	auto load = [&] { return loader.LoadStructures(file, &images, &textures, &materials, &lights, &geometry, &controllers, &animations, &scene); };
	Check(save() && load(), "empty Collada cache round trip");
	geometry.resize(1);
	geometry[0].msId = "triangle";
	geometry[0].mvIndexVec = {0,1,2}; geometry[0].mvVertexVec.resize(3); geometry[0].mvTangents.assign(12, 0);
	for(auto &v : geometry[0].mvVertexVec) { v.pos = 0; v.norm = cVector3f(0,0,1); v.tex = 0; }
	geometry[0].mvExtraVtxVec.resize(3);
	for(int i = 0; i < 3; ++i) geometry[0].mvExtraVtxVec[i].push_back(cColladaExtraVtx(i,i,i,i));
	controllers.resize(1);
	controllers[0].msId = "skin"; controllers[0].msTarget = "triangle";
	controllers[0].m_mtxBindShapeMatrix = cMatrixf::Identity;
	controllers[0].mvJoints = {"bone"}; controllers[0].mvWeights = {0.25f};
	controllers[0].mvMatrices = {cMatrixf::Identity}; controllers[0].mvPairs.resize(3);
	for(auto &pairs : controllers[0].mvPairs) pairs.push_back(cColladaJointPair(0,0));
	Check(save() && load() && geometry[0].mvIndexVec.size() == 3 && controllers[0].mvPairs.size() == 3, "skinned geometry cache round trip");
	TiXmlDocument document(file.c_str()); Check(document.LoadFile(), "read cache fixture");
	auto savedGeometry=geometry; auto savedControllers=controllers;
	document.RootElement()->RemoveAttribute("GeometryVersion");
	Check(document.SaveFile() && !load(), "old polygon topology cache rejected");
	geometry=savedGeometry; controllers=savedControllers;
	Check(save() && load(), "rebuild topology cache");
	Check(document.LoadFile(), "reread topology cache");
	auto *pairs = document.RootElement()->FirstChildElement("ControllerRoot")->FirstChildElement()->FirstChildElement("PairVec");
	pairs->FirstChildElement("PairNum")->FirstChild()->SetValue("4 1 1");
	Check(document.SaveFile() && !load() && controllers.empty(), "Collada pair count overrun");
	Check(document.LoadFile(), "restore cache fixture");
	pairs = document.RootElement()->FirstChildElement("ControllerRoot")->FirstChildElement()->FirstChildElement("PairVec");
	pairs->FirstChildElement("PairNum")->FirstChild()->SetValue("1 1 1");
	Check(document.SaveFile() && load(), "restored skinned geometry cache");
	Check(document.LoadFile(), "read restored cache fixture");
	auto *vertex = document.RootElement()->FirstChildElement("GeometryRoot")->FirstChildElement()->FirstChildElement("VertexVec");
	vertex->FirstChildElement("Positions")->FirstChild()->SetValue("1 2");
	Check(document.SaveFile() && !load() && geometry.empty(), "truncated Collada array clears partial results");
	Check(save(), "save empty cache");
	Check(document.LoadFile(), "reread cache fixture");
	document.RootElement()->FirstChildElement("ImageRoot")->SetAttribute("Size", 1);
	Check(document.SaveFile() && !load(), "Collada child count mismatch");
	std::ofstream(file) << "<ColladaCache/>";
	Check(!load(), "missing Collada cache sections");
	// Exercise the public import flow: a newer corrupt cache must rebuild from DAE.
	auto source = directory / "source.dae";
	auto cache = directory / "source.collcach";
	std::ofstream(source) << "<COLLADA><asset><up_axis>Y_UP</up_axis></asset></COLLADA>";
	std::ofstream(cache) << "<ColladaCache/>";
	std::filesystem::last_write_time(source, std::filesystem::last_write_time(cache) - std::chrono::seconds(2));
	loader.SetCacheDir(directory.string() + "/");
	Check(loader.FillStructures(source.string(), &images, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, true), "invalid cache falls back to source DAE");
	Check(loader.LoadStructures(cache.string(), &images, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr), "source import replaces invalid cache");
}

// Exercise the real upload path without a window. The CPU editing contract is
// deliberately separate from the interleaved GPU layout.
static void TestVertexUploads()
{
	auto gen = glad_glGenBuffers; auto del = glad_glDeleteBuffers;
	auto bind = glad_glBindBuffer; auto data = glad_glBufferData;
	glad_glGenBuffers = [](GLsizei n, GLuint* out) { static GLuint next=1; while(n--) *out++ = next++; };
	glad_glDeleteBuffers = [](GLsizei, const GLuint*) {};
	glad_glBindBuffer = [](GLenum, GLuint) {};
	glad_glBufferData = [](GLenum, GLsizeiptr, const void*, GLenum) {};
	{
		const auto flags = eVertexFlag_Position | eVertexFlag_Normal | eVertexFlag_TexCoord | eVertexFlag_Color0;
		cVertexBufferVBO buffer(nullptr, flags, eVertexBufferDrawType_Tri, eVertexBufferUsageType_Dynamic, 0, 0);
		for(int v=0; v<3; ++v) {
			buffer.AddVertex(eVertexFlag_Position, cVector3f(v==1, v==2, 0));
			buffer.AddVertex(eVertexFlag_Normal, cVector3f(0,0,1));
			buffer.AddVertex(eVertexFlag_TexCoord, cVector3f(v==1, v==2, 0));
			buffer.AddColor(eVertexFlag_Color0, cColor(0.2f,0.3f,0.4f,0.5f));
			buffer.AddIndex(v);
		}
		Check(buffer.Compile(eVertexCompileFlag_CreateTangents), "compile interleaved vertices with generated tangents");
		Check(buffer.mlStride == 18 && buffer.mvInterleaved.size() == 54, "interleaved vertex stride");
		auto verify = [&] {
			for(int v=0; v<3; ++v) for(int i=0; i<klNumOfVertexFlags; ++i)
				for(int c=0; c<kvVertexElements[i]; ++c)
					Check(buffer.mvInterleaved[v*buffer.mlStride+buffer.mvOffsets[i]+c] ==
						buffer.GetArray(kvVertexFlags[i])[v*kvVertexElements[i]+c], "all uploaded fields match packed editing arrays");
		};
		verify();
		float* positions = buffer.GetArray(eVertexFlag_Position);
		positions[4] = 7;
		buffer.GetArray(eVertexFlag_Tangent)[7] = -1;
		buffer.UpdateData(eVertexFlag_Position | eVertexFlag_Tangent, false);
		Check(positions == buffer.GetArray(eVertexFlag_Position), "dynamic upload preserves editing pointers");
		verify();
		std::unique_ptr<iVertexBuffer> copy(buffer.CreateCopy(eVertexBufferUsageType_Stream));
		Check(copy->GetArray(eVertexFlag_Tangent)[7] == -1 && copy->GetArray(eVertexFlag_Position)[4] == 7,
			"mesh copy preserves positions and tangent handedness");
		buffer.GetArray(eVertexFlag_Color0)[3] = 0.8f;
		buffer.UpdateData(eVertexFlag_Color0, false); verify();
		Check(copy->GetArray(eVertexFlag_Color0)[3] == 0.5f, "mesh copy owns independent editing data");
	}
	glad_glGenBuffers=gen; glad_glDeleteBuffers=del; glad_glBindBuffer=bind; glad_glBufferData=data;
}

class TestChannel : public iSoundChannel
{
public:
	TestChannel() : iSoundChannel(nullptr, nullptr) { mb3D = true; mlId = 0; }
	void Play() override {}
	void Stop() override {}
	void SetPaused(bool v) override { mbPaused = v; }
	void SetSpeed(float v) override { mfSpeed = v; }
	void SetVolume(float v) override { mfVolume = v; }
	void SetLooping(bool v) override { mbLooping = v; }
	void SetPan(float v) override { mfPan = v; }
	void Set3D(bool v) override { mb3D = v; }
	void SetPriority(int v) override { mlPriority = v; }
	int GetPriority() override { return mlPriority; }
	void SetPositionRelative(bool v) override { mbPositionRelative = v; }
	void SetPosition(const cVector3f &v) override { mvPosition = v; }
	void SetVelocity(const cVector3f &v) override { mvVelocity = v; }
	void SetMinDistance(float v) override { mfMinDistance = v; }
	void SetMaxDistance(float v) override { mfMaxDistance = v; }
	bool IsPlaying() override { return true; }
	bool IsBufferUnderrun() override { return false; }
	double GetElapsedTime() override { return 0; }
	double GetTotalTime() override { return 1; }
	void SetFiltering(bool, int) override {}
	void SetFilterGain(float) override {}
	void SetFilterGainHF(float) override {}
};

static void TestSound()
{
	cLowLevelSoundOpenAL lowLevel;
	lowLevel.m_mtxListener = cMatrixf::Identity;
	lowLevel.mvListenerPosition = 0;
	cSoundHandler handler(&lowLevel, nullptr);
	Check(handler.mlIdCount == 0, "initialized sound channel ID");
	cWorld3D world("test", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
	handler.SetWorld3D(&world);
	TestChannel channel;
	channel.SetMinDistance(2); channel.SetMaxDistance(10);
	cSoundEntry entry;
	entry.mpSound = &channel; entry.mfBlockMul = 1;
	handler.mfVolume = 0.25f;
	for(bool relative : {false, true})
		for(eSoundDest effect : {eSoundDest_World, eSoundDest_Gui})
			for(eSoundDest affected : {eSoundDest_World, eSoundDest_Gui})
			{
				channel.SetPositionRelative(relative); entry.mEffectType = effect;
				handler.mAffectedByVolume = affected;
				handler.UpdateDistanceVolume3D(&entry, 0, false, eSoundDest_World);
				Check(Near(channel.GetVolume(), effect == affected ? 0.25f : 1), "3D selective volume mask");
			}
	cMusicHandler music(&lowLevel, nullptr);
	music.mpMainSong = new cMusicEntry();
	music.mpMainSong->msFileName = "test";
	music.mpMainSong->mpStream = new TestChannel();
	Check(music.Play("test", 1, 0.1f, true) && music.mpMainSong->mpStream->GetLooping(), "same-song loop change");
	Check(music.Play("test", 0.5f, 0.1f, false) && !music.mpMainSong->mpStream->GetLooping(), "loop change with volume change");
}

// Newton 2 finds no contact between convex shapes with the same centre
// (a script asking if an entity it put in the middle of an area is in it)
static void TestCollideSameCentre()
{
	cLowLevelPhysicsNewton lowLevel;
	cPhysicsWorldNewton world;
	cMatrixf mtxOffset = cMath::MatrixRotateZ(kPi2f);
	iCollideShape *pCylinder = world.CreateCylinderShape(0.3f, 1.75f, &mtxOffset);
	iCollideShape *pBox = world.CreateBoxShape(cVector3f(0.36f, 3.66f, 4.7f), nullptr);
	for(int lAngle = 0; lAngle < 360; lAngle += 10)
	{
		cMatrixf mtxBox = cMath::MatrixRotateY(cMath::ToRad((float)lAngle));
		mtxBox.SetTranslation(cVector3f(1, -0.94f, -5));
		cMatrixf mtxCylinder = cMatrixf::Identity;
		mtxCylinder.SetTranslation(cVector3f(1, -0.94f, -5));
		cCollideData collideData;
		collideData.SetMaxSize(1);
		Check(world.CheckShapeCollision(pCylinder, mtxCylinder, pBox, mtxBox, collideData, 1), "collision of shapes with the same centre");
	}
}

// A scripted limit must move an island that Newton has already put to sleep.
static void TestSleepingJointLimits()
{
	cLowLevelPhysicsNewton lowLevel;
	for(int kind = 0; kind < 3; ++kind)
	for(bool minimum : {false, true})
	{
		cPhysicsWorldNewton world;
		world.SetWorld3D(nullptr);
		world.SetWorldSize(cVector3f(-10), cVector3f(10));
		auto *body = world.CreateBody("joint child", world.CreateBoxShape(cVector3f(0.5f), nullptr));
		body->SetMass(1);
		body->SetGravity(false);
		iPhysicsJoint *joint;
		const cVector3f pivot(0), pin(0, 0, 1);
		if(kind == 0) joint = world.CreateJointHinge("hinge", pivot, pin, nullptr, body);
		else if(kind == 1) joint = world.CreateJointSlider("slider", pivot, pin, nullptr, body);
		else joint = world.CreateJointScrew("screw", pivot, pin, nullptr, body);
		auto setLimits = [&](float min, float max)
		{
			if(kind == 0) {
				auto *hinge = static_cast<iPhysicsJointHinge*>(joint);
				hinge->SetMinAngle(min); hinge->SetMaxAngle(max);
			} else if(kind == 1) {
				auto *slider = static_cast<iPhysicsJointSlider*>(joint);
				slider->SetMinDistance(min); slider->SetMaxDistance(max);
			} else {
				auto *screw = static_cast<iPhysicsJointScrew*>(joint);
				screw->SetMinDistance(min); screw->SetMaxDistance(max);
			}
		};
		setLimits(-1, 1);
		for(int i = 0; i < 600; ++i) world.Simulate(1.0f / 60);
		Check(body->IsSleeping(), "joint fixture settles before its scripted limit changes");
		setLimits(-1, 1);
		Check(body->IsSleeping(), "unchanged joint limits preserve sleep");
		setLimits(minimum ? 0.4f : -1, minimum ? 1 : -0.4f);
		Check(!body->IsSleeping(), "changed joint limit wakes its sleeping child");
		for(int i = 0; i < 120; ++i) world.Simulate(1.0f / 60);
		const float position = kind == 0 ? joint->GetAngle() : joint->GetDistance();
		Check(minimum ? position > 0.2f : position < -0.2f, "sleeping joint moves toward its new limit");
	}
}

// Maps create some bodies outside the initial box, then resize the world.
static void TestExpandedPhysicsWorld()
{
	cLowLevelPhysicsNewton lowLevel;
	cPhysicsWorldNewton world;
	world.SetWorld3D(nullptr);
	world.SetWorldSize(cVector3f(20, -10, -10), cVector3f(40, 10, 10));
	auto *body = world.CreateBody("outside initial world", world.CreateBoxShape(cVector3f(1), nullptr));
	body->SetMass(1);
	for(int i = 0; i < 10; ++i) world.Simulate(1.0f / 60);
	Check(Near(body->GetWorldPosition().y, 0), "body outside the initial world is inactive");
	const float height = body->GetWorldPosition().y;
	world.SetWorldSize(cVector3f(-100), cVector3f(100));
	for(int i = 0; i < 60; ++i) world.Simulate(1.0f / 60);
	Check(body->GetWorldPosition().y < height - 1, "expanded world simulates its previously excluded body");
}

class TestProgram : public iGpuProgram
{
public:
	TestProgram() : iGpuProgram("test") {}
	bool Reload() override { return false; }
	void Unload() override {}
	void Destroy() override {}
	bool CreateFromFile(const tString&, const tString&) override { return false; }
	void Bind() override {}
	void UnBind() override {}
	bool SetTexture(const tString&, iTexture*, bool) override { return false; }
	bool SetTextureToUnit(int, iTexture*) override { return false; }
};

static void TestProgramParameters()
{
	TestProgram a, b;
	a.ParametersUploaded();
	a.SetFloat(eProgramParam::fogStart, 0);
	Check(!a.ParametersChanged(), "unchanged material data does not upload again");
	a.SetFloat(eProgramParam::fogStart, 3);
	Check(a.ParametersChanged(), "changed material data needs upload");
	const unsigned fog = static_cast<unsigned>(eProgramParam::fogStart) * 4;
	Check(a.GetParameterData()[fog] == 3 && b.GetParameterData()[fog] == 0, "program parameter blocks are isolated");
	a.ParametersUploaded();
	a.SetFloat(eProgramParam::fogStart, 3);
	Check(!a.ParametersChanged(), "identical material update remains clean");
	a.SetVec4f(eProgramParam::haloLook, 7, 8, 9, 10);
	a.SetVec4f(iGpuProgram::Element(eProgramParam::haloProfile, 31), 1, 2, 3, 4);
	Check(a.GetParameterData()[static_cast<unsigned>(eProgramParam::haloLook) * 4] == 7, "last array element preserves following member");
	cMatrixf matrix = cMatrixf::Identity;
	for(unsigned row=0; row<4; ++row)
		for(unsigned col=0; col<4; ++col) matrix.m[row][col] = float(row * 4 + col);
	a.SetMatrixf(eProgramParam::worldViewProj, matrix);
	const float* data = a.GetParameterData() + static_cast<unsigned>(eProgramParam::worldViewProj) * 4;
	for(unsigned row=0; row<4; ++row)
		for(unsigned col=0; col<4; ++col)
			Check(data[col * 4 + row] == matrix.m[row][col], "std140 matrix keeps column-major order");
	Check(!a.SetFloat(eProgramParam::Count, 1), "invalid parameter cannot overwrite block");
	Check(!a.SetMatrixf(eProgramParam::t, matrix), "matrix cannot overrun block");
}

class TestRenderMaterial : public iMaterial
{
public:
	TestRenderMaterial(bool abTransparent, bool abDepthTest)
		: iMaterial("test material", nullptr, nullptr, nullptr, nullptr, eMaterialPicture_Texture, nullptr)
	{
		mbIsTransparent = abTransparent;
		mbDepthTest = abDepthTest;
	}
	void Compile() override {}
	bool UsesType(eMaterialRenderType aType) override { return aType == eMaterialRenderType_Light; }
	int GetNumOfPasses(eMaterialRenderType aType, iLight3D*) override { return aType == eMaterialRenderType_Light ? 1 : 0; }
	eMaterialAlphaMode GetAlphaMode(eMaterialRenderType, int, iLight3D*) override { return eMaterialAlphaMode_Solid; }
	eMaterialBlendMode GetBlendMode(eMaterialRenderType, int, iLight3D*) override { return eMaterialBlendMode_Alpha; }
	eMaterialChannelMode GetChannelMode(eMaterialRenderType, int, iLight3D*) override { return eMaterialChannelMode_RGBA; }
};

class TestRefractionMaterial : public TestRenderMaterial
{
public:
	TestRefractionMaterial() : TestRenderMaterial(true, true) {}
	~TestRefractionMaterial() override
	{
		// These pointers are test tokens, not owned textures.
		SetTexture(nullptr, eMaterialTexture_Diffuse);
		SetTexture(nullptr, eMaterialTexture_Specular);
	}
	bool GetRefractionUsesDiffuse() override { return true; }
	eMaterialTexture GetRefractionDiffuseTexture() override { return eMaterialTexture_Specular; }
};

class TestRenderable : public iRenderable
{
public:
	TestRenderable(const tString &asName, iMaterial *apMaterial, iVertexBuffer *apVtx)
		: iRenderable(asName), mpMaterial(apMaterial), mpVtx(apVtx) {}
	iMaterial *GetMaterial() override { return mpMaterial; }
	iVertexBuffer *GetVertexBuffer() override { return mpVtx; }
	bool IsShadowCaster() override { return false; }
	cMatrixf *GetModelMatrix(cCamera3D*) override { return nullptr; }
	eRenderableType GetRenderType() override { return eRenderableType_Normal; }
	tString GetEntityType() override { return "TestRenderable"; }
	int GetMatrixUpdateCount() override { return 0; }
private:
	iMaterial *mpMaterial;
	iVertexBuffer *mpVtx;
};

class TestTexture : public iTexture
{
public:
	TestTexture(const tString &asName, iLowLevelGraphics *apLow)
		: iTexture(asName, "test", nullptr, apLow, eTextureType_Normal, false, eTextureTarget_1D) {}
	bool CreateFromBitmap(iBitmap2D*) override { return true; }
	bool CreateCubeFromBitmapVec(tBitmap2DVec*) override { return true; }
	bool Create(unsigned int, unsigned int, cColor) override { return true; }
	bool CreateAnimFromBitmapVec(tBitmap2DVec*) override { return true; }
	bool CreateFromArray(unsigned char*, int, const cVector3l&) override { return true; }
	void Update(float) override {}
	void SetPixels2D(int, const cVector2l&, const cVector2l&, eColorDataFormat, void*) override {}
	void KeepAlphaCoverage(float) override {}
	float GetGamma() override { return 1; }
	void SetGamma(float) override {}
	int GetHandle() override { return 0; }
	void SetWrapS(eTextureWrap aMode) override { mWrapS = aMode; }
	void SetWrapT(eTextureWrap aMode) override { mWrapT = aMode; }
	void SetWrapR(eTextureWrap aMode) override { mWrapR = aMode; }
	bool HasAnimation() override { return false; }
	void NextFrame() override {}
	void PrevFrame() override {}
	float GetT() override { return 0; }
	float GetAnimationFramePosition() override { return 0; }
	void SetAnimationFramePosition(float) override {}
	int GetCurrentLowlevelHandle() override { return 0; }
};

// Just enough resource ownership for the light fixture's falloff texture.
class TestTextureManager : public cTextureManager
{
public:
	using cTextureManager::cTextureManager;
	void Add(iResourceBase *apResource) { AddResource(apResource); }
};

class TestRenderResources : public cResources
{
public:
	TestRenderResources(iLowLevelResources *apLowLevelResources, iLowLevelGraphics *apLowLevelGraphics,
		cGraphics *apGraphics)
		: cResources(apLowLevelResources, apLowLevelGraphics)
	{
		mpLowLevelSystem = nullptr;
		mpImageManager = nullptr; mpGpuProgramManager = nullptr;
		mpParticleManager = nullptr; mpSoundManager = nullptr;
		mpFontManager = nullptr; mpScriptManager = nullptr;
		mpSoundEntityManager = nullptr; mpAnimationManager = nullptr;
		mpVideoManager = nullptr; mpMeshManager = nullptr;
		mpMaterialManager = nullptr; mpMeshLoaderHandler = nullptr;
		mpTextureManager = hplNew(TestTextureManager, (apGraphics, this));
		TestTexture *pFalloff = hplNew(TestTexture, ("core_falloff_linear.BMP", apLowLevelGraphics));
		// Keep the fixture texture alive between light bursts; the manager
		// releases the remaining users at teardown.
		for(int i = 0; i < 256; ++i) pFalloff->IncUserCount();
		static_cast<TestTextureManager *>(mpTextureManager)->Add(pFalloff);
	}
};

static void TestTransparentOrder()
{
	cLowLevelGraphicsSDL low;
	cGraphics graphics(&low, nullptr);
	cRenderList list(&graphics);
	cCamera3D camera;
	list.SetCamera(&camera);
	TestVertexBuffer vertices(1);
	TestRenderMaterial depthMaterial(true, true);
	TestRenderMaterial noDepthMaterial(true, false);

	TestRenderable back("back", &depthMaterial, &vertices);
	TestRenderable front("front", &depthMaterial, &vertices);
	TestRenderable middle("middle", &depthMaterial, &vertices);
	TestRenderable disabled("disabled", &noDepthMaterial, &vertices);
	TestRenderable equalA("equalA", &depthMaterial, &vertices);
	TestRenderable equalB("equalB", &depthMaterial, &vertices);
	back.SetPosition(cVector3f(0, 0, -9));
	front.SetPosition(cVector3f(0, 0, -1));
	middle.SetPosition(cVector3f(0, 0, -5));
	disabled.SetPosition(cVector3f(0, 0, -20));
	equalA.SetPosition(cVector3f(0, 0, -5));
	equalB.SetPosition(cVector3f(0, 0, -5));

	list.Clear();
	Check(list.Add(&back) && list.Add(&equalA) && list.Add(&disabled) &&
		list.Add(&front) && list.Add(&equalB) && list.Add(&middle), "transparent objects enter the list");
	Check(!list.Add(&middle), "duplicate add is rejected by render count");
	list.Compile();
	Check(list.GetTransparentDrawNum() == 6, "duplicate add creates one draw");
	const char *expectedOrder[] = {"disabled", "back", "equalA", "equalB", "middle", "front"};
	for(int i = 0; i < 6; ++i)
		Check(list.GetTransparentDraw(i).mpObject->GetName() == expectedOrder[i],
			"transparent order is depth-test group, ascending Z, then submission order");
	Check(list.GetTransparentDraw(2).mpObject->GetZ() < list.GetTransparentDraw(3).mpObject->GetZ() ||
		list.GetTransparentDraw(2).mpObject->GetZ() == list.GetTransparentDraw(3).mpObject->GetZ(),
		"equal-depth pairs are adjacent");

	list.Clear();
	Check(list.Add(&front) && list.Add(&back) && list.Add(&middle), "clear accepts a new frame");
	list.Compile();
	const char *rebuiltOrder[] = {"back", "middle", "front"};
	for(int i = 0; i < 3; ++i)
		Check(list.GetTransparentDraw(i).mpObject->GetName() == rebuiltOrder[i],
			"clear/rebuild sorts the new submission order");

	// Water names the specular texture as its refraction source. The record
	// must keep that choice separate from the ordinary diffuse texture.
	int diffuseId = 0, specularId = 0;
	iTexture *pDiffuse = reinterpret_cast<iTexture *>(&diffuseId);
	iTexture *pSpecular = reinterpret_cast<iTexture *>(&specularId);
	TestRefractionMaterial water;
	water.SetTexture(pDiffuse, eMaterialTexture_Diffuse);
	water.SetTexture(pSpecular, eMaterialTexture_Specular);
	TestRenderable waterObject("water", &water, &vertices);
	waterObject.SetPosition(cVector3f(0, 0, -2));
	list.Clear();
	Check(list.Add(&waterObject), "refraction object enters the list");
	list.Compile();
	Check(list.GetTransparentDrawNum() == 1, "refraction object has one draw");
	const cTransparentDraw &refraction = list.GetTransparentDraw(0);
	Check(refraction.mpDiffuse == pDiffuse && refraction.mpRefractionDiffuse == pSpecular,
		"refraction records the material-selected texture, not the diffuse texture");
}

static void TestLightList()
{
	cLowLevelGraphicsSDL low;
	cLowLevelResourcesSDL lowResources(&low);
	cGraphics graphics(&low, nullptr);
	cRenderList list(&graphics);
	cCamera3D camera;
	list.SetCamera(&camera);
	TestRenderResources resources(&lowResources, &low, &graphics);
	TestVertexBuffer vertices(1);
	TestRenderMaterial material(false, true);
	TestRenderable nearObject("near", &material, &vertices);
	TestRenderable farObject("far", &material, &vertices);
	nearObject.SetPosition(cVector3f(0));
	farObject.SetPosition(cVector3f(100, 0, 0));

	auto run = [&](int alLights)
	{
		std::vector<std::unique_ptr<cLight3DPoint>> lights;
		list.Clear();
		for(int i = 0; i < alLights; ++i)
		{
			lights.emplace_back(new cLight3DPoint("light", &resources));
			lights.back()->SetPosition(cVector3f(0));
			lights.back()->SetFarAttenuation(10);
			list.Add(lights.back().get());
		}
		list.Add(&nearObject);
		list.Add(&farObject);
		list.Compile();
		Check(list.GetLightNum() == alLights, "compiled light count matches the visible lights");
		for(int i = 0; i < list.GetLightNum(); ++i)
		{
			iLight3D *pLight = list.GetLight(i);
			Check(pLight != nullptr && list.GetLightObjects(i) == 1, "only the intersecting object joins each light");
			cRenderQueue *pQueue = list.GetQueue(eMaterialRenderType_Light, i);
			Check(pQueue && pQueue->mvDraws.size() == 1, "each light owns its one-pass draw queue");
			Check(pQueue->mvDraws[0].mState.mpLight == pLight, "queue draw stays associated with its light");
		}
	};

	run(0);
	run(1);
	run(30);
	run(31);
	run(64);
	run(1);
	run(64);

	// A sector-restricted light and an unrestricted one: the object's sector
	// decides which queue receives it.
	list.Clear();
	cLight3DPoint restricted("restricted", &resources);
	cLight3DPoint unrestricted("unrestricted", &resources);
	restricted.SetPosition(cVector3f(0));
	unrestricted.SetPosition(cVector3f(0));
	restricted.SetFarAttenuation(10);
	unrestricted.SetFarAttenuation(10);
	restricted.SetOnlyAffectInSector(true);
	restricted.SetCurrentSector(reinterpret_cast<cSector *>(1));
	nearObject.SetCurrentSector(reinterpret_cast<cSector *>(2));
	list.Add(&restricted);
	list.Add(&unrestricted);
	list.Add(&nearObject);
	list.Compile();
	Check(list.GetLightNum() == 2, "sector fixture keeps both lights");
	for(int i = 0; i < 2; ++i)
	{
		const bool bRestricted = list.GetLight(i)->GetName() == "restricted";
		Check(list.GetLightObjects(i) == (bRestricted ? 0 : 1),
			"sector restriction excludes the object from only that light's queue");
	}
}

static void TestDrawQueue()
{
	cRenderDraw first, next;
	Check(first.FirstChangedState(nullptr) == eRenderStateType_Sector, "first draw applies all states");
	Check(next.FirstChangedState(&first) == eRenderStateType_Render, "equal keys still draw");
	next.mKey[5] = 1;
	Check(next.FirstChangedState(&first) == eRenderStateType_BlendMode, "blend key reapplies following state");
	next = first; next.mKey[6] = 1;
	Check(next.FirstChangedState(&first) == eRenderStateType_Program, "program change reapplies textures and matrices");
	next = first; next.mKey[9 + MAX_TEXTUREUNITS] = 1;
	Check(next.FirstChangedState(&first) == eRenderStateType_Matrix, "matrix change preserves earlier material state");
	next = first; next.mKey[7] = 1;
	Check(next.FirstChangedState(&first) == eRenderStateType_Program, "custom setup change reapplies material and matrix values");
	next = first; next.mKey[0] = 1;
	Check(next.FirstChangedState(&first) == eRenderStateType_Sector, "sector change reapplies ambient setup");
	cRenderQueue queue;
	for(int i=0; i<4; ++i){
		queue.mvDraws.emplace_back();
		queue.mvDraws.back().mKey[1] = i % 2;
		queue.mvDraws.back().mState.mlPass = i;
	}
	queue.Sort();
	const int expected[] = {0, 2, 1, 3};
	for(int i=0; i<4; ++i) Check(queue.mvDraws[i].mState.mlPass == expected[i], "pass ordering retains equal-key draws in submission order");
	queue.Clear();
	Check(queue.mvDraws.empty(), "cleared queue keeps no previous draws");
}

static void TestSprites()
{
	cSpriteBatch batch;
	std::array<cSpriteVertex,4> corners;
	for(int i=0; i<4; ++i)
		corners[i] = cSpriteVertex(cVector3f(i, i+10, i+20), cColor(i*0.1f, 0.2f, 0.3f, 0.4f), cVector3f(i*0.25f,0.5f,0));
	const cSpriteBlend alpha(eBlendFunc_SrcAlpha,eBlendFunc_OneMinusSrcAlpha);
	const cSpriteBlend additive(eBlendFunc_One,eBlendFunc_One);
	// These distinct opaque handles are compared, never dereferenced.
	int textureIds[2];
	iTexture* textureA = reinterpret_cast<iTexture*>(&textureIds[0]);
	iTexture* textureB = reinterpret_cast<iTexture*>(&textureIds[1]);
	batch.Add(textureA, alpha, corners);
	const int topology[] = {0,1,2,0,2,3};
	for(int i=0; i<6; ++i)
	{
		const auto& vertex = batch.mvVertices[i];
		const auto& original = corners[topology[i]];
		Check(vertex.pos == original.pos && vertex.tex == original.tex && vertex.col == original.col,
			"sprite triangles retain winding, depth, colour and UV coordinates");
	}
	batch.Add(textureA, alpha, corners);
	Check(batch.mvBatches.size() == 1 && batch.mvBatches[0].mlCount == 12, "adjacent sprites share one batch");
	batch.Add(textureB, alpha, corners);
	batch.Add(textureA, alpha, corners);
	batch.Add(textureA, additive, corners);
	batch.Add(textureA, cSpriteBlend(eBlendFunc_One,eBlendFunc_One,false), corners);
	Check(batch.mvBatches.size() == 5, "texture, blend factors and blend enable delimit batches without reordering");
	for(size_t i=1; i<batch.mvBatches.size(); ++i)
		Check(batch.mvBatches[i].mlFirst == batch.mvBatches[i-1].mlFirst + batch.mvBatches[i-1].mlCount,
			"batch ranges cover triangles in submission order");
	for(int i=0; i<20000; ++i) batch.Add(textureA, alpha, corners);
	Check(batch.mvVertices.size() == 20006*6 && batch.mvBatches.back().mlCount == 20000*6,
		"sprites grow beyond the old fixed array capacity without dropping triangles");
	cSpriteBatch empty;
	empty.Flush(nullptr);

	tGfxBufferSet hud;
	tGuiRenderObjectSet gui;
	for(int i=0; i<5; ++i)
	{
		cGfxBufferObject object{};
		object.mvTransform = cVector3f(i,0,i == 3 ? -1 : 2);
		hud.insert(object);
		cGuiRenderObject guiObject{};
		guiObject.mvPos = object.mvTransform;
		gui.insert(guiObject);
	}
	const int order[] = {3,0,1,2,4};
	int i = 0;
	for(const auto& object : hud) Check(object.mvTransform.x == order[i++], "HUD sorts depth and retains submission order at equal Z");
	i = 0;
	for(const auto& object : gui) Check(object.mvPos.x == order[i++], "GUI sorts depth and retains submission order at equal Z");
	Check(cGuiMaterial_Diffuse(nullptr).GetBlend() == cSpriteBlend(eBlendFunc_One,eBlendFunc_Zero), "GUI replace blend");
	Check(cGuiMaterial_Alpha(nullptr).GetBlend() == alpha && cGuiMaterial_FontNormal(nullptr).GetBlend() == alpha, "GUI alpha and glyph blend");
	Check(cGuiMaterial_Additive(nullptr).GetBlend() == additive, "GUI additive blend");
	Check(cGuiMaterial_Modulative(nullptr).GetBlend() == cSpriteBlend(eBlendFunc_DestColor,eBlendFunc_Zero), "GUI modulative blend");
}

int hplMain(const hpl::tString&)
{
	auto directory = std::filesystem::temp_directory_path() / ("overture-regressions-" + std::to_string(getpid()));
	std::filesystem::create_directories(directory);
	try
	{
		{ cLowLevelGraphicsSDL graphics; Check(graphics.GetWindow()==nullptr, "backend lifetime without a GL context"); }
		TestStrings(); TestDirectoryScan(directory); TestSkinning(); TestPathfinding(directory); TestTangents();
		TestAnimation(directory); TestColladaCache(directory); TestSound(); TestCollideSameCentre();
		TestSleepingJointLimits(); TestExpandedPhysicsWorld();
		TestEffectInstances(); TestProgramParameters(); TestDrawQueue(); TestSprites(); TestVertexUploads(); TestColladaPolygons();
		TestTransparentOrder(); TestLightList();
		cPidControllerf pid(1,1,1,0);
		for(int i = 0; i < 9; ++i) Check(std::isfinite(pid.Output(1, 0.1f)), "zero-history PID");
		pid.SetErrorNum(10); for(int i = 0; i < 9; ++i) pid.Output(1, 0.1f);
		pid.SetErrorNum(2); Check(std::isfinite(pid.Output(1, 0.1f)), "resized PID history");
		Check(std::isfinite(pid.Output(1, 0)), "zero PID timestep");
		cPidControllerf defaultPid; Check(defaultPid.Output(1, 0.1f) == 0, "default PID");
		std::filesystem::remove_all(directory);
		std::cout << "Engine regressions passed\n";
		return 0;
	}
	catch(const std::exception &error)
	{
		std::cerr << error.what() << '\n';
		std::filesystem::remove_all(directory);
		return 1;
	}
}
