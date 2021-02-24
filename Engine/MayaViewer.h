#ifndef MayaViewer_H_
#define MayaViewer_H_

#include "gameplay.h"
#include "ComLib.h"

using namespace gameplay;

struct VertexHandlerData
{
    char m_meshName[50];
    int m_dataIndex;
};

/**
 * Main game class.
 */
class MayaViewer: public Game
{
public:

    /**
     * Constructor.
     */
    MayaViewer();

    /**
     * @see Game::keyEvent
     */
	void keyEvent(Keyboard::KeyEvent evt, int key);
	
    /**
     * @see Game::touchEvent
     */
    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

	// mouse events
	bool mouseEvent(Mouse::MouseEvent evt, int x, int y, int wheelDelta);


protected:

    /**
     * @see Game::initialize
     */
    void initialize();

    /**
     * @see Game::finalize
     */
    void finalize();

    /**
     * @see Game::update
     */
    void update(float elapsedTime);

    /**
     * @see Game::render
     */
    void render(float elapsedTime);

private:

    /**
     * Draws the scene each frame.
     */
    bool drawScene(Node* node);

    //Consumer
    ComLib* consumer;

    //Messages
    ViewerMessage::BaseMessage baseMSG;
    ViewerMessage::MeshMessage meshMSG;
    std::vector<ViewerMessage::VertexMessage> vertexMSG;
    ViewerMessage::TransformMessage transformMSG;
    ViewerMessage::RenamedMessage renamedMSG;
    ViewerMessage::MaterialMessage materialMSG;
    ViewerMessage::DeletedMessage deletedMSG;
    ViewerMessage::CameraTransformMessage cameraTransformMSG;
    ViewerMessage::CameraMessage cameraMSG;
    ViewerMessage::SingleVertexMessage singleVertexMSG;
    ViewerMessage::TopologyChangedMessage topoChangeMSG;
    ViewerMessage::FilePathMessage filePathMSG;
    //

    //Light
    Node* lightNode;

    //Models
    std::vector<VertexHandlerData> vertexHandler;
    std::vector<std::vector<float>> meshVertexData;
    int meshCount = 0;

    int findVertexHandler(char meshName[]);

    void createMeshAndAddToScene();
    void setMatrix();
    void renameNode();
    void changeMeshMaterial();
    void deleteNode();
    void moveCamera();
    void updateSingleVertex();
    void updateChangedTopology();
    void updateCameraProjection();
    void updateTexture();

    Scene* _scene;
    bool _wireframe;
};

#endif
