#include "MayaViewer.h"

// Declare our game instance
MayaViewer game;

//constexpr int gModelCount = 1;
static bool gKeys[256] = {};
int gDeltaX;
int gDeltaY;
bool gMousePressed;

MayaViewer::MayaViewer()
    : _scene(NULL), _wireframe(false)
{
	DWORD totalMemSize = 200 * 1024;
	consumer = new ComLib("mainFileMap", totalMemSize, ComEnum::TYPE::CONSUMER);
}

void MayaViewer::initialize()
{

    // Load game scene from file
	_scene = Scene::create();

	Camera* cam = Camera::createPerspective(45.0f, getAspectRatio(), 1.0, 100.0f);
	Node* cameraNode = _scene->addNode("camera");
	cameraNode->setCamera(cam);
	_scene->setActiveCamera(cam);
	SAFE_RELEASE(cam);

	cameraNode->translate(0, 0, 30);
	cameraNode->rotateX(MATH_DEG_TO_RAD(0.f));

	lightNode = _scene->addNode("light");
	Light* light = Light::createPoint(Vector3(0.5f, 0.5f, 0.5f), 20);
	lightNode->setLight(light);
	SAFE_RELEASE(light);
	lightNode->translate(Vector3(0, 1, 5));
}

void MayaViewer::finalize()
{
	delete consumer;

	vertexHandler.clear();
	meshVertexData.clear();

    SAFE_RELEASE(_scene);
}

void MayaViewer::update(float elapsedTime)
{

	for (int i = 0; i < 8; i++)
	{
		if (consumer->getNextBaseMessage(baseMSG))
		{
			if (consumer->consumerCanRead(baseMSG))
			{
				switch (baseMSG.m_msgType)
				{
				case ViewerMessage::MSG_TYPE::Mesh:
					consumer->readMeshMessage(meshMSG, vertexMSG);
					createMeshAndAddToScene();
					vertexMSG.clear();
					break;
				case ViewerMessage::MSG_TYPE::Transform:
					consumer->readTransformMessage(transformMSG);
					setMatrix();
					break;
				case ViewerMessage::MSG_TYPE::Camera:
					consumer->readCameraMessage(cameraMSG);
					updateCameraProjection();
					break;
				case ViewerMessage::MSG_TYPE::CameraTransform:
					consumer->readCameraTransformMessage(cameraTransformMSG);
					moveCamera();
					break;
				case ViewerMessage::MSG_TYPE::SingleVertex:
					consumer->readSingleVertexMessage(singleVertexMSG);
					updateSingleVertex();
					break;
				case ViewerMessage::MSG_TYPE::TopologyChanged:
					consumer->readTopologyChangedMessage(topoChangeMSG, vertexMSG);
					updateChangedTopology();
					break;
				case ViewerMessage::MSG_TYPE::Rename:
					consumer->readRenamedMessage(renamedMSG);
					renameNode();
					break;
				case ViewerMessage::MSG_TYPE::Material:
					consumer->readMaterialMessage(materialMSG);
					changeMeshMaterial();
					break;
				case ViewerMessage::MSG_TYPE::FilePath:
					consumer->readFilePathMessage(filePathMSG);
					updateTexture();
					break;
				case ViewerMessage::MSG_TYPE::Delete:
					consumer->readDeletedMessage(deletedMSG);
					deleteNode();
					break;
				}
			}
		}
	}

	Node* camnode = _scene->getActiveCamera()->getNode();
	if (gKeys[Keyboard::KEY_W])
		camnode->translateForward(0.5);
	if (gKeys[Keyboard::KEY_S])
		camnode->translateForward(-0.5);
	if (gKeys[Keyboard::KEY_A])
		camnode->translateLeft(0.5);
	if (gKeys[Keyboard::KEY_D])
		camnode->translateLeft(-0.5);

	if (gMousePressed) {
		camnode->rotate(camnode->getRightVectorWorld(), MATH_DEG_TO_RAD(gDeltaY / 10.0));
		camnode->rotate(camnode->getUpVectorWorld(), MATH_DEG_TO_RAD(gDeltaX / 5.0));
	}
}

void MayaViewer::render(float elapsedTime)
{
    // Clear the color and depth buffers
    clear(CLEAR_COLOR_DEPTH, Vector4(0.1f,0.0f,0.0f,1.0f), 1.0f, 0);

    // Visit all the nodes in the scene for drawing
    _scene->visit(this, &MayaViewer::drawScene);
}

bool MayaViewer::drawScene(Node* node)
{
    // If the node visited contains a drawable object, draw it
    Drawable* drawable = node->getDrawable(); 
    if (drawable)
        drawable->draw(_wireframe);

    return true;
}

void MayaViewer::keyEvent(Keyboard::KeyEvent evt, int key)
{
    if (evt == Keyboard::KEY_PRESS)
    {
		gKeys[key] = true;
        switch (key)
        {
        case Keyboard::KEY_ESCAPE:
            exit();
            break;
		};
    }
	else if (evt == Keyboard::KEY_RELEASE)
	{
		gKeys[key] = false;
	}
}

bool MayaViewer::mouseEvent(Mouse::MouseEvent evt, int x, int y, int wheelDelta)
{
	static int lastX = 0;
	static int lastY = 0;
	gDeltaX = lastX - x;
	gDeltaY = lastY - y;
	lastX = x;
	lastY = y;
	gMousePressed =
		(evt == Mouse::MouseEvent::MOUSE_PRESS_LEFT_BUTTON) ? true :
		(evt == Mouse::MouseEvent::MOUSE_RELEASE_LEFT_BUTTON) ? false : gMousePressed;

	return true;
}

void MayaViewer::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex)
{
    switch (evt)
    {
    case Touch::TOUCH_PRESS:
        _wireframe = !_wireframe;
        break;
    case Touch::TOUCH_RELEASE:
        break;
    case Touch::TOUCH_MOVE:
        break;
    };
}

int MayaViewer::findVertexHandler(char meshName[])
{
	int result = -1;

	for (int i = 0; i < vertexHandler.size(); i++)
	{
		if (strcmp(vertexHandler[i].m_meshName, meshName) == 0)
		{
			result = i;
			break;
		}
	}

	return result;
}

void MayaViewer::createMeshAndAddToScene()
{
	Node* existingNode = _scene->findNode(meshMSG.m_meshName);
	if (!existingNode)
	{
		const int size = vertexMSG.size() * 8;

		unsigned int vertexCount = meshMSG.m_nrOfVertices;
		unsigned int indexCount = meshMSG.m_nrOfVertices;

		float* vertices = new float[size];
		short* indices = new short[vertexMSG.size()];

		int currentIndex = 0;

		for (int i = 0; i < vertexMSG.size(); i++)
		{
			vertices[currentIndex] = vertexMSG[i].m_position[0];
			currentIndex++;
			vertices[currentIndex] = vertexMSG[i].m_position[1];
			currentIndex++;
			vertices[currentIndex] = vertexMSG[i].m_position[2];
			currentIndex++;

			vertices[currentIndex] = vertexMSG[i].m_uv[0];
			currentIndex++;
			vertices[currentIndex] = vertexMSG[i].m_uv[1];
			currentIndex++;

			vertices[currentIndex] = vertexMSG[i].m_normal[0];
			currentIndex++;
			vertices[currentIndex] = vertexMSG[i].m_normal[1];
			currentIndex++;
			vertices[currentIndex] = vertexMSG[i].m_normal[2];
			currentIndex++;

			indices[i] = vertexMSG[i].m_index;
		}

		VertexFormat::Element elements[] =
		{
			VertexFormat::Element(VertexFormat::POSITION, 3),
			VertexFormat::Element(VertexFormat::TEXCOORD0, 2),
			VertexFormat::Element(VertexFormat::NORMAL, 3)
		};

		Mesh* mesh = Mesh::createMesh(VertexFormat(elements, 3), vertexCount, true);
		if (mesh == NULL)
		{
			GP_ERROR("Failed to create mesh.");
		}
		else
		{
			mesh->setVertexData(vertices, 0, vertexCount);

			Model* model = Model::create(mesh);

			Material* mats = model->setMaterial("res/shaders/colored.vert", "res/shaders/colored.frag", "POINT_LIGHT_COUNT 1");

			mats->setParameterAutoBinding("u_worldViewProjectionMatrix", "WORLD_VIEW_PROJECTION_MATRIX");
			mats->setParameterAutoBinding("u_inverseTransposeWorldViewMatrix", "INVERSE_TRANSPOSE_WORLD_VIEW_MATRIX");
			mats->getParameter("u_diffuseColor")->setValue(Vector4(0.5f, 0.5f, 0.5f, 1.0f));
			mats->getParameter("u_pointLightColor[0]")->setValue(lightNode->getLight()->getColor());
			mats->getParameter("u_pointLightPosition[0]")->bindValue(lightNode, &Node::getTranslationWorld);
			mats->getParameter("u_pointLightRangeInverse[0]")->bindValue(lightNode->getLight(), &Light::getRangeInverse);
			mats->getStateBlock()->setCullFace(true);
			mats->getStateBlock()->setDepthTest(true);
			mats->getStateBlock()->setDepthWrite(true);

			char nodeName[50] = {};
			strcpy_s(nodeName, meshMSG.m_meshName);
			Node* node = _scene->addNode(nodeName);
			node->setDrawable(model);
			SAFE_RELEASE(model);
		}

		//Set Mesh Data for Single Vertex Changes
		VertexHandlerData empty;
		empty.m_dataIndex = meshCount;
		strcpy_s(empty.m_meshName, meshMSG.m_meshName);
		vertexHandler.push_back(empty);

		std::vector<float> emptyFloats;
		meshVertexData.push_back(emptyFloats);
		for (int i = 0; i < size; i++)
		{
			meshVertexData[meshCount].push_back(vertices[i]);
		}
		meshCount++;

		delete[] vertices;
		delete[] indices;
	}

}

void MayaViewer::setMatrix()
{
	Matrix matrix;
	matrix.set(transformMSG.m_matrix);

	Vector3 trans;
	Quaternion rot;
	Vector3 scale;

	matrix.getTranslation(&trans);
	matrix.getRotation(&rot);
	matrix.getScale(&scale);

	Node* node = _scene->findNode(transformMSG.m_meshName);
	if (node) {

		node->setTranslation(trans);
		node->setRotation(rot);
		node->setScale(scale);
	}

}

void MayaViewer::renameNode()
{
	Node* node = _scene->findNode(renamedMSG.m_oldName);
	if (node)
	{
		node->setId(renamedMSG.m_newName);
		int index = findVertexHandler(renamedMSG.m_oldName);
		if (index != -1)
		{
			strcpy_s(vertexHandler[index].m_meshName, renamedMSG.m_newName);
		}
	}
}

void MayaViewer::changeMeshMaterial()
{
	Node* node = _scene->findNode(materialMSG.m_meshName);
	if (node)
	{
		Model* model = (Model*)node->getDrawable();
		Material* mat = model->getMaterial();
		if (materialMSG.m_shadingType == ViewerMessage::SHADING_TYPE::COLORED)
		{
			mat = model->setMaterial("res/shaders/colored.vert", "res/shaders/colored.frag", "POINT_LIGHT_COUNT 1");
			mat->setParameterAutoBinding("u_worldViewProjectionMatrix", "WORLD_VIEW_PROJECTION_MATRIX");
			mat->setParameterAutoBinding("u_inverseTransposeWorldViewMatrix", "INVERSE_TRANSPOSE_WORLD_VIEW_MATRIX");

			mat->getParameter("u_pointLightColor[0]")->setValue(lightNode->getLight()->getColor());
			mat->getParameter("u_pointLightPosition[0]")->bindValue(lightNode, &Node::getTranslationWorld);
			mat->getParameter("u_pointLightRangeInverse[0]")->bindValue(lightNode->getLight(), &Light::getRangeInverse);

			mat->getParameter("u_diffuseColor")->setValue(Vector4(materialMSG.m_diffuse[0], materialMSG.m_diffuse[1], materialMSG.m_diffuse[2], materialMSG.m_diffuse[3]));
			mat->getParameter("u_ambientColor")->setValue(Vector3(materialMSG.m_ambient[0], materialMSG.m_ambient[1], materialMSG.m_ambient[2]));

			mat->getStateBlock()->setCullFace(true);
			mat->getStateBlock()->setDepthTest(true);
			mat->getStateBlock()->setDepthWrite(true);
		}
		else if (materialMSG.m_shadingType == ViewerMessage::SHADING_TYPE::TEXTURED)
		{
			mat = model->setMaterial("res/shaders/textured.vert", "res/shaders/textured.frag", "POINT_LIGHT_COUNT 1");

			mat->setParameterAutoBinding("u_worldViewProjectionMatrix", "WORLD_VIEW_PROJECTION_MATRIX");
			mat->setParameterAutoBinding("u_inverseTransposeWorldViewMatrix", "INVERSE_TRANSPOSE_WORLD_VIEW_MATRIX");

			mat->getParameter("u_pointLightColor[0]")->setValue(lightNode->getLight()->getColor());
			mat->getParameter("u_pointLightPosition[0]")->bindValue(lightNode, &Node::getTranslationWorld);
			mat->getParameter("u_pointLightRangeInverse[0]")->bindValue(lightNode->getLight(), &Light::getRangeInverse);

			mat->getParameter("u_ambientColor")->setValue(Vector3(materialMSG.m_ambient[0], materialMSG.m_ambient[1], materialMSG.m_ambient[2]));

			mat->getParameter("_baseColor")->setValue(Vector4(materialMSG.m_diffuse[0], materialMSG.m_diffuse[1], materialMSG.m_diffuse[2], materialMSG.m_diffuse[3]));

			if (strcmp(materialMSG.m_diffTexturePath, "") != 0)
			{
				Texture::Sampler* sampler = mat->getParameter("u_diffuseTexture")->setValue(materialMSG.m_diffTexturePath, true);
				sampler->setFilterMode(Texture::LINEAR_MIPMAP_LINEAR, Texture::LINEAR);
			}

			mat->getStateBlock()->setCullFace(true);
			mat->getStateBlock()->setDepthTest(true);
			mat->getStateBlock()->setDepthWrite(true);
		}

	}
}

void MayaViewer::deleteNode()
{
	Node* node = _scene->findNode(deletedMSG.m_deletedMeshName);
	if (node)
	{
		node->removeAllChildren();
		_scene->removeNode(node);
	}
}

void MayaViewer::moveCamera()
{

	Matrix matrix;
	matrix.set(cameraTransformMSG.m_matrix);

	Vector3 trans;
	Quaternion rot;
	Vector3 scale;

	matrix.getTranslation(&trans);
	matrix.getRotation(&rot);
	matrix.getScale(&scale);

	Node* camNode = _scene->getActiveCamera()->getNode();
	camNode->setTranslation(trans);
	camNode->setRotation(rot);
	camNode->setScale(scale);

}

void MayaViewer::updateSingleVertex()
{
	Node* node = _scene->findNode(singleVertexMSG.m_meshName);
	if (node)
	{
		Model* model = (Model*)node->getDrawable();
		Mesh* mesh = model->getMesh();

		//Update Vertex
		int index = findVertexHandler(singleVertexMSG.m_meshName);

		if (index != -1)
		{
			int vertexIndex;
			if (singleVertexMSG.m_index == 0)
			{
				vertexIndex = 0;
			}
			else
			{
				vertexIndex = (8 * singleVertexMSG.m_index);
			}

			int simplifiedIndex = vertexHandler[index].m_dataIndex;

			meshVertexData[simplifiedIndex][vertexIndex] = singleVertexMSG.m_position[0];
			vertexIndex++;
			meshVertexData[simplifiedIndex][vertexIndex] = singleVertexMSG.m_position[1];
			vertexIndex++;
			meshVertexData[simplifiedIndex][vertexIndex] = singleVertexMSG.m_position[2];
			vertexIndex++;

			//UV
			meshVertexData[simplifiedIndex][vertexIndex] = singleVertexMSG.m_uv[0];
			vertexIndex++;
			meshVertexData[simplifiedIndex][vertexIndex] = singleVertexMSG.m_uv[1];
			vertexIndex++;

			//Normal
			meshVertexData[simplifiedIndex][vertexIndex] = singleVertexMSG.m_normal[0];
			vertexIndex++;
			meshVertexData[simplifiedIndex][vertexIndex] = singleVertexMSG.m_normal[1];
			vertexIndex++;
			meshVertexData[simplifiedIndex][vertexIndex] = singleVertexMSG.m_normal[2];

			int vertexCount = meshVertexData[simplifiedIndex].size() / 8;

			mesh->setVertexData(meshVertexData[simplifiedIndex].data(), 0, vertexCount);

		}
	}
}

void MayaViewer::updateChangedTopology()
{
	Node* node = _scene->findNode(topoChangeMSG.m_meshName);
	if (node)
	{
		//Update Vertex Data
		const int size = vertexMSG.size() * 8;

		unsigned int vertexCount = topoChangeMSG.m_nrOfVertices;
		unsigned int indexCount = topoChangeMSG.m_nrOfVertices;

		float* vertices = new float[size];
		short* indices = new short[vertexMSG.size()];

		int currentIndex = 0;

		for (int i = 0; i < vertexMSG.size(); i++)
		{
			vertices[currentIndex] = vertexMSG[i].m_position[0];
			currentIndex++;
			vertices[currentIndex] = vertexMSG[i].m_position[1];
			currentIndex++;
			vertices[currentIndex] = vertexMSG[i].m_position[2];
			currentIndex++;

			vertices[currentIndex] = vertexMSG[i].m_uv[0];
			currentIndex++;
			vertices[currentIndex] = vertexMSG[i].m_uv[1];
			currentIndex++;

			vertices[currentIndex] = vertexMSG[i].m_normal[0];
			currentIndex++;
			vertices[currentIndex] = vertexMSG[i].m_normal[1];
			currentIndex++;
			vertices[currentIndex] = vertexMSG[i].m_normal[2];
			currentIndex++;

			indices[i] = vertexMSG[i].m_index;
		}

		//Set Mesh Data for Single Vertex Changes
		int index = findVertexHandler(topoChangeMSG.m_meshName);

		if (index != -1)
		{
			int simplifiedIndex = vertexHandler[index].m_dataIndex;

			//Delete old data
			meshVertexData[simplifiedIndex].clear();

			//Add new data
			for (int i = 0; i < size; i++)
			{
				meshVertexData[simplifiedIndex].push_back(vertices[i]);
			}

			Model* model = (Model*)node->getDrawable();
			Mesh* mesh = model->getMesh();

			VertexFormat::Element elements[] =
			{
				VertexFormat::Element(VertexFormat::POSITION, 3),
				VertexFormat::Element(VertexFormat::TEXCOORD0, 2),
				VertexFormat::Element(VertexFormat::NORMAL, 3)
			};

			Mesh* newMesh = Mesh::createMesh(VertexFormat(elements, 3), vertexCount, true);
			if (mesh == NULL)
			{
				GP_ERROR("Failed to create mesh.");
			}
			else
			{

				newMesh->setVertexData(meshVertexData[simplifiedIndex].data(), 0, vertexCount);

				Model* newModel = Model::create(newMesh);
				newModel->setMaterial(model->getMaterial());
				node->setDrawable(newModel);

				SAFE_RELEASE(newModel);

			}
		}

		delete[] vertices;
		delete[] indices;

	}
}

void MayaViewer::updateCameraProjection()
{
	if (cameraMSG.m_projectionType == ViewerMessage::PROJECTION_TYPE::ORTHOGRAPHIC)
	{
		//Ortho
		Camera* cam = Camera::createOrthographic(20.0f, 10.0f, getAspectRatio(), 1.0, 2000.0f);

		Node* oldCam = _scene->findNode("camera");
		oldCam->setCamera(cam);
		_scene->setActiveCamera(cam);
	}
	else
	{
		//Persp
		Camera* cam = Camera::createPerspective(45.0f, getAspectRatio(), 1.0, 100.0f);

		Node* oldCam = _scene->findNode("camera");
		oldCam->setCamera(cam);
		_scene->setActiveCamera(cam);
	}
}

void MayaViewer::updateTexture()
{
	Node* node = _scene->findNode(filePathMSG.m_meshName);
	if (node)
	{
		Model* model = (Model*)node->getDrawable();
		Material* mat = model->getMaterial();

		if (strcmp(filePathMSG.m_diffTexturePath, "") != 0)
		{
			Texture::Sampler* sampler = mat->getParameter("u_diffuseTexture")->setValue(filePathMSG.m_diffTexturePath, true);
			sampler->setFilterMode(Texture::LINEAR_MIPMAP_LINEAR, Texture::LINEAR);
		}
	}
}
