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
#include <unistd.h>

#include "math/Math.h"
#include "math/PidController.h"
#include "graphics/VertexBuffer.h"
#include "graphics/Animation.h"
#include "graphics/AnimationTrack.h"
#include "graphics/Mesh.h"
#include "graphics/Skeleton.h"
#include "graphics/Bone.h"
#include "impl/Platform.h"
#include "resources/MeshManager.h"
#include "system/String.h"
#include "tinyxml.h"

// Exercise search/cache internals without a physics world or graphics context.
// These tests still call the actual engine methods, including CPU skinning.
#define private public
#define protected public
#include "ai/AStar.h"
#include "ai/AINodeContainer.h"
#include "graphics/SubMesh.h"
#include "impl/MeshLoaderCollada.h"
#include "impl/LowLevelSoundOpenAL.h"
#include "resources/Resources.h"
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
	void CreateShadowDouble(bool) override {}
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
		if(flag == eVertexFlag_Texture1) return tangents.data();
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
		mpImageManager = nullptr; mpGpuProgramManager = nullptr; mpTileSetManager = nullptr;
		mpImageEntityManager = nullptr; mpParticleManager = nullptr; mpSoundManager = nullptr;
		mpFontManager = nullptr; mpScriptManager = nullptr; mpTextureManager = nullptr;
		mpMaterialManager = nullptr; mpSoundEntityManager = nullptr; mpAnimationManager = nullptr;
		mpVideoManager = nullptr; mpMeshManager = nullptr; mpMeshLoaderHandler = nullptr;
	}
};

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
	Check(subEntity->mpDynVtxBuffer->GetArray(eVertexFlag_Texture1)[7] == -1, "unweighted tangent handedness");
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
	TiXmlDocument oldSave((directory / "animation.xml").c_str());
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

int hplMain(const hpl::tString&)
{
	auto directory = std::filesystem::temp_directory_path() / ("overture-regressions-" + std::to_string(getpid()));
	std::filesystem::create_directories(directory);
	try
	{
		TestStrings(); TestDirectoryScan(directory); TestSkinning(); TestPathfinding(directory); TestTangents();
		TestAnimation(directory); TestColladaCache(directory); TestSound();
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
