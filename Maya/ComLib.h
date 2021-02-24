#pragma once
#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <Windows.h>
#include <time.h>
#include <assert.h>
#include <vector>
#include "MessageGuide.h"

namespace ComEnum
{
	enum TYPE { PRODUCER, CONSUMER };

	//Max message size + 4 for producer, 4 for consumer
	const int ALLOWED_DIST = 14;

	const DWORD START_DELAY = 1;

	const int MAX_MESSAGE_SIZE = 10;
}

class ComLib
{
private:

	ComEnum::TYPE m_type;

	//Memory Leak Log
	HANDLE m_hLogFile;

	//Main File Map
	HANDLE m_hFileMap;
	void* m_mData;
	size_t m_bufferSize;
	LPCSTR m_fileMapName;

	//Secondary File Map
	HANDLE m_hFileMapSecond;
	void* m_secondData;
	size_t m_secondBuffSize;
	LPCSTR m_secondMapName;

	int* m_offset;
	int* m_otherOffset;

	int m_allowedDist;


public:

	//Constructor
	ComLib(LPCSTR secret, const size_t& buffSize, ComEnum::TYPE type);

	//Destructor
	~ComLib();

    ViewerMessage::BaseMessage getNextMessageDetails();

    //NEW AND APPROVED___________________________________________

    int getOffset() { return *m_offset; }
    int getOtherOffset() { return *m_otherOffset; }

    //CONSUMER___________________________________________________
    //Update Consumer
    bool consumerCanRead(ViewerMessage::BaseMessage baseMSG);

    //Read Message Consumer
    bool getNextBaseMessage(ViewerMessage::BaseMessage& baseMSG);
    void readMeshMessage(ViewerMessage::MeshMessage& meshMSG, std::vector<ViewerMessage::VertexMessage>& vertexMSG);
    void readTransformMessage(ViewerMessage::TransformMessage& transformMSG);
    void readRenamedMessage(ViewerMessage::RenamedMessage& renamedMSG);
    void readMaterialMessage(ViewerMessage::MaterialMessage& materialMSG);
    void readDeletedMessage(ViewerMessage::DeletedMessage& deletedMSG);
    void readCameraMessage(ViewerMessage::CameraMessage& cameraMSG);
    void readCameraTransformMessage(ViewerMessage::CameraTransformMessage& cameraTransformMSG);
    void readSingleVertexMessage(ViewerMessage::SingleVertexMessage& singleVertexMSG);
    void readTopologyChangedMessage(ViewerMessage::TopologyChangedMessage& topoChangeMSG, std::vector<ViewerMessage::VertexMessage>& vertexMSG);
    void readFilePathMessage(ViewerMessage::FilePathMessage& filePathMSG);

    //PRODUCER___________________________________________________
    //Update Producer
    bool producerCanWrite(ViewerMessage::BaseMessage baseMSG);

    //Write Message Producer
    void writeMeshMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::MeshMessage meshMSG, std::vector<ViewerMessage::VertexMessage> vertexMSG);
    void writeTransformMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::TransformMessage transformMSG);
    void writeRenamedMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::RenamedMessage renamedMSG);
    void writeMaterialMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::MaterialMessage materialMSG);
    void writeDeletedMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::DeletedMessage deletedMSG);
    void writeCameraMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::CameraMessage cameraMSG);
    void writeCameraTransformMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::CameraTransformMessage cameraTransformMSG);
    void writeSingleVertexMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::SingleVertexMessage singleVertexMSG);
    void writeTopologyChangedMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::TopologyChangedMessage topoChangeMSG, std::vector<ViewerMessage::VertexMessage> vertexMSG);
    void writeFilePathMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::FilePathMessage filePathMSG);
};
