#include "ComLib.h"

ComLib::ComLib(LPCSTR secret, const size_t& buffSize, ComEnum::TYPE type)
{
	m_type = type;
    m_bufferSize = buffSize;
    m_fileMapName = secret;
    m_offset = new int(0);
    m_otherOffset = new int(0);

    //Variables Secondary FileMap
    m_secondBuffSize = 2 * sizeof(int);
    m_secondMapName = "secondFileMap";

    if (m_type == ComEnum::TYPE::PRODUCER)
    {
        m_allowedDist = ComEnum::ALLOWED_DIST;

        //MAIN FileMap______________________

        //Create/Get FileMapping
        m_hFileMap = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            (DWORD)0,
            m_bufferSize,
            secret);
        
        //File View
        m_mData = MapViewOfFile(m_hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        //__________________________________

        //SECONDARY FileMap_________________

        //Create FileMapping
        m_hFileMapSecond = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            (DWORD)0,
            m_secondBuffSize,
            m_secondMapName);

        //File View
        m_secondData = MapViewOfFile(m_hFileMapSecond, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        //__________________________________

    }
    else
    {
        m_allowedDist = 1;

        //Start delay here:
        Sleep(ComEnum::START_DELAY);

        //MAIN FileMap______________________

        m_hFileMap = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            (DWORD)0,
            m_bufferSize,
            secret);

        //File View
        m_mData = MapViewOfFile(m_hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        //__________________________________

        //SECONDARY FileMap_________________

        m_hFileMapSecond = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            (DWORD)0,
            m_secondBuffSize,
            m_secondMapName);

        //File View
        m_secondData = MapViewOfFile(m_hFileMapSecond, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        //__________________________________

    }
}

ComLib::~ComLib()
{
    delete m_offset;
    delete m_otherOffset;

    UnmapViewOfFile((LPCVOID)m_mData);
    CloseHandle(m_hFileMap);

    UnmapViewOfFile((LPCVOID)m_secondData);
    CloseHandle(m_hFileMapSecond);
}

ViewerMessage::BaseMessage ComLib::getNextMessageDetails()
{
    ViewerMessage::BaseMessage result(ViewerMessage::Base);

    if (*m_offset + sizeof(ViewerMessage::BaseMessage) > m_bufferSize)
    {
        //Not enough space left in buffer to be information

        //Get producer position from secondary buffer
        memcpy(m_otherOffset, m_secondData, sizeof(int));


        //Check if producer is in front of us at buffer end
        if (*m_otherOffset >= *m_offset)
        {
            //Delay until producer resets
            while (*m_otherOffset >= *m_offset)
            {
                //Get producer position from secondary buffer
                memcpy(m_otherOffset, m_secondData, sizeof(int));
            }

        }

        // Check if able to jump to beginning of buffer
        if (*m_otherOffset > m_allowedDist)
        {
            //Jump to start of buffer
            *m_offset = 0;
        }
        else
        {

            //Delay
            while (*m_otherOffset <= m_allowedDist)
            {
                //Get producer position from secondary buffer
                memcpy(m_otherOffset, m_secondData, sizeof(int));
            }

            //Jump to start of buffer
            *m_offset = 0;

        }

    }
    else
    {
        //Enough space

        //Get producer position from secondary buffer
        memcpy(m_otherOffset, m_secondData, sizeof(int));

        if (*m_otherOffset == *m_offset)
        {
            //If they are in the same spot, Consumer has to wait

            //Delay until producer is far enough ahead
            while (abs(*m_offset - *m_otherOffset) < m_allowedDist)
            {
                //Get producer position from secondary buffer
                memcpy(m_otherOffset, m_secondData, sizeof(int));
            }
        }
        else if (*m_otherOffset > * m_offset)
        {
            //Producer is in front of consumer

            //Check if producer is far enough ahead
            if (!(abs(*m_offset - *m_otherOffset) >= m_allowedDist))
            {
                //Producer is not far enough ahead

                //Delay
                while (abs(*m_offset - *m_otherOffset) < m_allowedDist)
                {
                    //Get producer position from secondary buffer
                    memcpy(m_otherOffset, m_secondData, sizeof(int));
                }
            }

        }

    }

    //Reading is allowed

    //Read message size
    memcpy(&result, (char*)m_mData + *m_offset, sizeof(ViewerMessage::BaseMessage));

    //Increase offset
    *m_offset += sizeof(ViewerMessage::BaseMessage);

    //Update Position
    memcpy((char*)m_secondData + sizeof(int), m_offset, sizeof(int));

    return result;
}



bool ComLib::consumerCanRead(ViewerMessage::BaseMessage baseMSG)
{

    bool result = false;

    bool dummy = false;

    if (baseMSG.m_msgType == ViewerMessage::MSG_TYPE::Dummy || baseMSG.m_msgType == -1)
    {
        dummy = true;
    }

    size_t updatedLength = baseMSG.m_messageSize;

    //Check if dummy message

    if (dummy)
    {

        //This was a dummy message

        //Get producer position from secondary buffer
        memcpy(m_otherOffset, m_secondData, sizeof(int));


        //Check if producer is in front of us at buffer end
        if (*m_otherOffset >= *m_offset)
        {
            result = false;
            return result;

        }

        // Check if able to jump to beginning of buffer
        if (*m_otherOffset > m_allowedDist)
        {
            //Jump to start of buffer
            *m_offset = 0;

            //Update own position
            memcpy((char*)m_secondData + sizeof(int), m_offset, sizeof(int));

            result = true;
            return result;
        }
        else
        {

            result = false;
            return result;

        }

        //ONLY IN DUMMY MESSAGE SCENARIO_______

        //Read message size
        baseMSG = getNextMessageDetails();

        updatedLength = baseMSG.m_messageSize;
        //_____________________________________

        result = true;
        return result;

    }
    else
    {
        //This was not a dummy message, continue as usual

        //Check if buffer ends after this message
        if (*m_offset + sizeof(int) + (sizeof(char) * updatedLength) == m_bufferSize)
        {
            //Buffer ends after this message

            //Get producer position from secondary buffer
            memcpy(m_otherOffset, m_secondData, sizeof(int));

            if (*m_otherOffset < *m_offset)
            {
                //Producer is behind consumer

                //Reading is allowed

                result = true;
                return result;

            }
            else if (*m_otherOffset == *m_offset)
            {
                //If they are in the same spot, Consumer has to wait

                result = false;
                return result;

            }
            else
            {
                //Producer is in front of consumer

                //Check if producer is far enough ahead
                if (abs(*m_offset - *m_otherOffset) > m_allowedDist)
                {
                    //Producer is far enough ahead

                    //Reading is allowed

                    result = true;
                    return result;

                }
                else
                {
                    //Producer is not far enough ahead

                    result = false;
                    return result;

                }

            }

            //Buffer has ended, jump to start

            //Get producer position from secondary buffer
            memcpy(m_otherOffset, m_secondData, sizeof(int));


            //Check if producer is in front of us at buffer end
            if (*m_otherOffset >= *m_offset)
            {
                result = false;
                return result;

            }

            // Check if able to jump to beginning of buffer
            if (*m_otherOffset > m_allowedDist)
            {

                //Jump to start of buffer
                *m_offset = 0;

                //Update own position
                memcpy((char*)m_secondData + sizeof(int), m_offset, sizeof(int));

                result = true;
                return result;
            }
            else
            {

                result = false;
                return result;

            }

        }
        else
        {

            //Buffer does not end after this message

            //Get producer position from secondary buffer
            memcpy(m_otherOffset, m_secondData, sizeof(int));

            //Check if we can read
            if (*m_otherOffset < *m_offset)
            {
                //Producer is behind consumer

                //Reading is allowed

                result = true;
                return result;

            }
            else if (*m_otherOffset == *m_offset)
            {
                //If they are in the same spot, Consumer has to wait

                result = false;
                return result;

            }
            else
            {
                //Producer is in front of consumer

                //Check if producer is far enough ahead
                if (abs(*m_offset - *m_otherOffset) > m_allowedDist)
                {
                    //Producer is far enough ahead

                    //Reading is allowed

                    result = true;
                    return result;

                }
                else
                {
                    //Producer is not far enough ahead

                    result = false;
                    return result;

                }

            }

        }


    }

    return result;
}

bool ComLib::getNextBaseMessage(ViewerMessage::BaseMessage& baseMSG)
{
    bool result = true;

    if (*m_offset + sizeof(ViewerMessage::BaseMessage) > m_bufferSize)
    {
        //Not enough space left in buffer to be information

        //Get producer position from secondary buffer
        memcpy(m_otherOffset, m_secondData, sizeof(int));


        //Check if producer is in front of us at buffer end
        if (*m_otherOffset >= *m_offset)
        {
            result = false;
            return result;

        }

        // Check if able to jump to beginning of buffer
        if (*m_otherOffset > m_allowedDist)
        {
            //Jump to start of buffer
            *m_offset = 0;

            //Update Position
            memcpy((char*)m_secondData + sizeof(int), m_offset, sizeof(int));

        }
        else
        {

            result = false;
            return result;

        }

    }
    else
    {
        //Enough space

        //Get producer position from secondary buffer
        memcpy(m_otherOffset, m_secondData, sizeof(int));

        if (*m_otherOffset == *m_offset)
        {
            //If they are in the same spot, Consumer has to wait

            result = false;
            return result;

        }
        else if (*m_otherOffset > * m_offset)
        {
            //Producer is in front of consumer

            //Check if producer is far enough ahead
            if (!(abs(*m_offset - *m_otherOffset) >= m_allowedDist))
            {
                //Producer is not far enough ahead

                result = false;
                return result;
            }

        }

    }

    //Reading is allowed

    //Read message size
    memcpy(&baseMSG, (char*)m_mData + *m_offset, sizeof(ViewerMessage::BaseMessage));

    //Increase offset
    *m_offset += sizeof(ViewerMessage::BaseMessage);

    //Update Position
    memcpy((char*)m_secondData + sizeof(int), m_offset, sizeof(int));

    result = true;

    return result;
}

void ComLib::readMeshMessage(ViewerMessage::MeshMessage& meshMSG, std::vector<ViewerMessage::VertexMessage>& vertexMSG)
{

    //Read Mesh Message
    memcpy(&meshMSG, (char*)m_mData + *m_offset, sizeof(ViewerMessage::MeshMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::MeshMessage);

    //Update Position
    memcpy((char*)m_secondData + sizeof(int), m_offset, sizeof(int));

    vertexMSG.resize(meshMSG.m_nrOfVertices);

    for (int i = 0; i < meshMSG.m_nrOfVertices; i++)
    {
        //Read Vertex Message
        memcpy(&vertexMSG[i], (char*)m_mData + *m_offset, sizeof(ViewerMessage::VertexMessage));

        //Offset
        *m_offset += sizeof(ViewerMessage::VertexMessage);

        //Update Position
        memcpy((char*)m_secondData + sizeof(int), m_offset, sizeof(int));
    }
}

void ComLib::readTransformMessage(ViewerMessage::TransformMessage& transformMSG)
{
    //Read Transform Message
    memcpy(&transformMSG, (char*)m_mData + *m_offset, sizeof(ViewerMessage::TransformMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::TransformMessage);

    //Update Position
    memcpy((char*)m_secondData + sizeof(int), m_offset, sizeof(int));
}

void ComLib::readRenamedMessage(ViewerMessage::RenamedMessage& renamedMSG)
{
    //Read Renamed Message
    memcpy(&renamedMSG, (char*)m_mData + *m_offset, sizeof(ViewerMessage::RenamedMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::RenamedMessage);

    //Update Position
    memcpy((char*)m_secondData + sizeof(int), m_offset, sizeof(int));
}

void ComLib::readMaterialMessage(ViewerMessage::MaterialMessage& materialMSG)
{
    //Read Material Message
    memcpy(&materialMSG, (char*)m_mData + *m_offset, sizeof(ViewerMessage::MaterialMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::MaterialMessage);

    //Update Position
    memcpy((char*)m_secondData + sizeof(int), m_offset, sizeof(int));
}

void ComLib::readDeletedMessage(ViewerMessage::DeletedMessage& deletedMSG)
{
    //Read Deleted Message
    memcpy(&deletedMSG, (char*)m_mData + *m_offset, sizeof(ViewerMessage::DeletedMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::DeletedMessage);

    //Update Position
    memcpy((char*)m_secondData + sizeof(int), m_offset, sizeof(int));
}

void ComLib::readCameraMessage(ViewerMessage::CameraMessage& cameraMSG)
{
    //Read Camera Message
    memcpy(&cameraMSG, (char*)m_mData + *m_offset, sizeof(ViewerMessage::CameraMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::CameraMessage);

    //Update Position
    memcpy((char*)m_secondData + sizeof(int), m_offset, sizeof(int));
}

void ComLib::readCameraTransformMessage(ViewerMessage::CameraTransformMessage& cameraTransformMSG)
{
    //Read Camera Transform Message
    memcpy(&cameraTransformMSG, (char*)m_mData + *m_offset, sizeof(ViewerMessage::CameraTransformMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::CameraTransformMessage);

    //Update Position
    memcpy((char*)m_secondData + sizeof(int), m_offset, sizeof(int));
}

void ComLib::readSingleVertexMessage(ViewerMessage::SingleVertexMessage& singleVertexMSG)
{
    //Read Single Vertex Message
    memcpy(&singleVertexMSG, (char*)m_mData + *m_offset, sizeof(ViewerMessage::SingleVertexMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::SingleVertexMessage);

    //Update Position
    memcpy((char*)m_secondData + sizeof(int), m_offset, sizeof(int));
}

void ComLib::readTopologyChangedMessage(ViewerMessage::TopologyChangedMessage& topoChangeMSG, std::vector<ViewerMessage::VertexMessage>& vertexMSG)
{
    //Read Topology Changed Message
    memcpy(&topoChangeMSG, (char*)m_mData + *m_offset, sizeof(ViewerMessage::TopologyChangedMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::TopologyChangedMessage);

    //Update Position
    memcpy((char*)m_secondData + sizeof(int), m_offset, sizeof(int));

    vertexMSG.resize(topoChangeMSG.m_nrOfVertices);

    for (int i = 0; i < topoChangeMSG.m_nrOfVertices; i++)
    {
        //Read Vertex Message
        memcpy(&vertexMSG[i], (char*)m_mData + *m_offset, sizeof(ViewerMessage::VertexMessage));

        //Offset
        *m_offset += sizeof(ViewerMessage::VertexMessage);

        //Update Position
        memcpy((char*)m_secondData + sizeof(int), m_offset, sizeof(int));
    }
}

void ComLib::readFilePathMessage(ViewerMessage::FilePathMessage& filePathMSG)
{
    //Read File Path Message
    memcpy(&filePathMSG, (char*)m_mData + *m_offset, sizeof(ViewerMessage::FilePathMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::FilePathMessage);

    //Update Position
    memcpy((char*)m_secondData + sizeof(int), m_offset, sizeof(int));
}

bool ComLib::producerCanWrite(ViewerMessage::BaseMessage baseMSG)
{
    bool result = false;

    m_allowedDist = baseMSG.m_messageSize + 1;

    if (*m_offset + baseMSG.m_messageSize <= m_bufferSize)
    {

        //Enough space in buffer

        //Get consumer position from secondary buffer
        memcpy(m_otherOffset, ((char*)m_secondData) + sizeof(int), sizeof(int));

        if (*m_otherOffset < *m_offset)
        {

            //Consumer is behind Producer

            //Writing is allowed
            result = true;
            return result;

        }
        else if (*m_otherOffset == *m_offset)
        {

            //If they are in the same spot, Consumer has to wait

            //Writing is allowed

            result = true;
            return result;

        }
        else
        {
            //Consumer is in front of Producer
            if (abs(*m_offset - *m_otherOffset) > m_allowedDist)
            {

                //Consumer is far enough ahead

                //Writing is allowed

                result = true;
                return result;

            }
            else
            {

                //Consumer is not far enough ahead, we risk overwriting unread information
                result = false;
                return result;

            }
        }

    }
    else
    {

        //Not enough space in buffer

        //Check if space for a dummy message
        if (*m_offset + sizeof(ViewerMessage::BaseMessage) > m_bufferSize)
        {
            //Not enough space for dummy message

            //m_offset is at buffer end

            //Get consumer position from secondary buffer
            memcpy(m_otherOffset, ((char*)m_secondData) + sizeof(int), sizeof(int));

            //Control if consumer is close to start of buffer
            if (*m_otherOffset > m_allowedDist)
            {

                //Consumer is far enough ahead

                //Jump to start of buffer
                *m_offset = 0;

                //Update position
                memcpy(m_secondData, m_offset, sizeof(int));

                //Writing is allowed

                result = true;
                return result;

            }
            else
            {

                //Consumer is not far enough ahead, we risk overwriting unread information

                result = false;
                return result;

            }


        }
        else
        {
            //Prepare dummy message
            ViewerMessage::DummyMessage dummy;

            //Get consumer position from secondary buffer
            memcpy(m_otherOffset, ((char*)m_secondData) + sizeof(int), sizeof(int));


            //Check if consumer is in front of us at buffer end
            if (*m_otherOffset > * m_offset)
            {

                result = false;
                return result;

            }


            //Control if consumer is close to start of buffer
            if (*m_otherOffset > m_allowedDist)
            {

                //Consumer is far enough ahead

                //Leave dummy message
                memcpy((char*)m_mData + *m_offset, &dummy, sizeof(ViewerMessage::DummyMessage));

                //Jump to start of buffer
                *m_offset = 0;

                //Update position
                memcpy(m_secondData, m_offset, sizeof(int));

                result = true;
                return result;

            }
            else
            {

                //Consumer is not far enough ahead, we risk overwriting unread information

                result = false;
                return result;

            }
        }
    }
    return result;
}

void ComLib::writeMeshMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::MeshMessage meshMSG, std::vector<ViewerMessage::VertexMessage> vertexMSG)
{

    //Write Base MSG
    memcpy((char*)m_mData + *m_offset, &baseMSG, sizeof(ViewerMessage::BaseMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::BaseMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));

    //Write Mesh MSG
    memcpy((char*)m_mData + *m_offset, &meshMSG, sizeof(ViewerMessage::MeshMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::MeshMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));

    for (int i = 0; i < vertexMSG.size(); i++)
    {
        //Write Vertex MSGs
        memcpy((char*)m_mData + *m_offset, &vertexMSG[i], sizeof(ViewerMessage::VertexMessage));

        //Offset
        *m_offset += sizeof(ViewerMessage::VertexMessage);

        //Update position
        memcpy(m_secondData, m_offset, sizeof(int));
    }

}

void ComLib::writeTransformMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::TransformMessage transformMSG)
{
    //Write Base MSG
    memcpy((char*)m_mData + *m_offset, &baseMSG, sizeof(ViewerMessage::BaseMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::BaseMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));

    //Write Transform MSG
    memcpy((char*)m_mData + *m_offset, &transformMSG, sizeof(ViewerMessage::TransformMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::TransformMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));
}

void ComLib::writeRenamedMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::RenamedMessage renamedMSG)
{
    //Write Base MSG
    memcpy((char*)m_mData + *m_offset, &baseMSG, sizeof(ViewerMessage::BaseMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::BaseMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));

    //Write Renamed MSG
    memcpy((char*)m_mData + *m_offset, &renamedMSG, sizeof(ViewerMessage::RenamedMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::RenamedMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));
}

void ComLib::writeMaterialMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::MaterialMessage materialMSG)
{
    //Write Base MSG
    memcpy((char*)m_mData + *m_offset, &baseMSG, sizeof(ViewerMessage::BaseMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::BaseMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));

    //Write Material MSG
    memcpy((char*)m_mData + *m_offset, &materialMSG, sizeof(ViewerMessage::MaterialMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::MaterialMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));
}

void ComLib::writeDeletedMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::DeletedMessage deletedMSG)
{
    //Write Base MSG
    memcpy((char*)m_mData + *m_offset, &baseMSG, sizeof(ViewerMessage::BaseMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::BaseMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));

    //Write Deleted MSG
    memcpy((char*)m_mData + *m_offset, &deletedMSG, sizeof(ViewerMessage::DeletedMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::DeletedMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));
}

void ComLib::writeCameraMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::CameraMessage cameraMSG)
{
    //Write Base MSG
    memcpy((char*)m_mData + *m_offset, &baseMSG, sizeof(ViewerMessage::BaseMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::BaseMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));

    //Write Camera MSG
    memcpy((char*)m_mData + *m_offset, &cameraMSG, sizeof(ViewerMessage::CameraMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::CameraMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));
}

void ComLib::writeCameraTransformMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::CameraTransformMessage cameraTransformMSG)
{
    //Write Base MSG
    memcpy((char*)m_mData + *m_offset, &baseMSG, sizeof(ViewerMessage::BaseMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::BaseMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));

    //Write Camera Transform MSG
    memcpy((char*)m_mData + *m_offset, &cameraTransformMSG, sizeof(ViewerMessage::CameraTransformMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::CameraTransformMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));
}

void ComLib::writeSingleVertexMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::SingleVertexMessage singleVertexMSG)
{
    //Write Base MSG
    memcpy((char*)m_mData + *m_offset, &baseMSG, sizeof(ViewerMessage::BaseMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::BaseMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));

    //Write Single Vertex MSG
    memcpy((char*)m_mData + *m_offset, &singleVertexMSG, sizeof(ViewerMessage::SingleVertexMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::SingleVertexMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));
}

void ComLib::writeTopologyChangedMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::TopologyChangedMessage topoChangeMSG, std::vector<ViewerMessage::VertexMessage> vertexMSG)
{
    //Write Base MSG
    memcpy((char*)m_mData + *m_offset, &baseMSG, sizeof(ViewerMessage::BaseMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::BaseMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));

    //Write Topology Changed MSG
    memcpy((char*)m_mData + *m_offset, &topoChangeMSG, sizeof(ViewerMessage::TopologyChangedMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::TopologyChangedMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));

    for (int i = 0; i < vertexMSG.size(); i++)
    {
        //Write Vertex MSGs
        memcpy((char*)m_mData + *m_offset, &vertexMSG[i], sizeof(ViewerMessage::VertexMessage));

        //Offset
        *m_offset += sizeof(ViewerMessage::VertexMessage);

        //Update position
        memcpy(m_secondData, m_offset, sizeof(int));
    }
}

void ComLib::writeFilePathMessage(ViewerMessage::BaseMessage baseMSG, ViewerMessage::FilePathMessage filePathMSG)
{
    //Write Base MSG
    memcpy((char*)m_mData + *m_offset, &baseMSG, sizeof(ViewerMessage::BaseMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::BaseMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));

    //Write Single Vertex MSG
    memcpy((char*)m_mData + *m_offset, &filePathMSG, sizeof(ViewerMessage::FilePathMessage));

    //Offset
    *m_offset += sizeof(ViewerMessage::FilePathMessage);

    //Update position
    memcpy(m_secondData, m_offset, sizeof(int));
}
