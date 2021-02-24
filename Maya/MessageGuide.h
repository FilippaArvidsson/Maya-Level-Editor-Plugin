#pragma once

namespace ViewerMessage
{
	enum MSG_TYPE
	{
		Base,
		Dummy,
		Mesh,
		Material,
		Transform,
		Rename,
		Delete,
		Camera,
		CameraTransform,
		SingleVertex,
		TopologyChanged,
		FilePath
	};

	enum SHADING_TYPE
	{
		COLORED,
		TEXTURED
	};

	enum PROJECTION_TYPE
	{
		PERSPECTIVE,
		ORTHOGRAPHIC
	};

	struct BaseMessage
	{
		MSG_TYPE m_msgType;
		int m_messageSize;

		BaseMessage(MSG_TYPE type)
		{
			m_msgType = type;
			m_messageSize = 0;
		}
		BaseMessage()
		{
			m_messageSize = 0;
			m_msgType = MSG_TYPE::Base;
		}
	};

	struct DummyMessage
	{
		int number = -1;
	};

	struct MeshMessage
	{
		//Name
		char m_meshName[50] = { ' ' };

		//Number of Vertices
		unsigned int m_nrOfVertices;

		MeshMessage()
		{
			m_nrOfVertices = 0;
		}
	};

	struct TransformMessage
	{
		//Mesh Name
		char m_meshName[50] = { ' ' };

		//Matrix
		float m_matrix[16];
	};

	struct SingleVertexMessage
	{
		char m_meshName[50] = { ' ' };
		float m_position[3];
		unsigned int m_index;
		float m_uv[2];
		float m_normal[3];
	};

	struct TopologyChangedMessage
	{
		char m_meshName[50] = { ' ' };
		unsigned int m_nrOfVertices = 0;
	};

	struct VertexMessage
	{
		float m_position[3];
		float m_uv[2];
		float m_normal[3];
		unsigned int m_index;
	};

	struct RenamedMessage
	{
		char m_oldName[50] = { ' ' };
		char m_newName[50] = { ' ' };
	};

	struct DeletedMessage
	{
		char m_deletedMeshName[50] = { ' ' };
	};

	struct MaterialMessage
	{
		//Color or Texture?
		SHADING_TYPE m_shadingType = SHADING_TYPE::COLORED;

		//Mesh Name
		char m_meshName[50] = { ' ' };

		//Variables
		float m_ambient[4];
		float m_diffuse[4];

		//Textures
		char m_diffTexturePath[100] = { ' ' };

	};

	struct FilePathMessage
	{
		//Mesh Name
		char m_meshName[50] = { ' ' };

		//Textures
		char m_diffTexturePath[100] = { ' ' };
	};

	struct CameraMessage
	{
		//Projection Type (Perspective/Orthographic)
		PROJECTION_TYPE m_projectionType;

		//Projection Matrix
		float m_projectionMatrix[16];
	};

	struct CameraTransformMessage
	{
		//Matrix
		float m_matrix[16];
	};

}