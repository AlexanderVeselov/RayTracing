#include "scene.hpp"
#include "mathlib.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>

Scene::Scene(const char* filename, size_t cell_resolution) : m_Filename(filename), m_CellResolution(cell_resolution), m_SceneBox(0.0f, 0.0f)
{
    LoadTriangles();
    CreateGrid();
}

void Scene::CreateGrid()
{
    //typedef std::vector<Triangle>::const_iterator iter_t;
    std::vector<cl_uint> lower_indices;
    std::vector<CellData> lower_cells;
    clock_t t = clock();
    int resolution = 2;
    std::cout << "Creating uniform grid" << std::endl;
    std::cout << "Scene bounding box min: " << m_SceneBox.min << " max: " << m_SceneBox.max << std::endl;
    std::cout << "Resolution: ";
    while (resolution <= m_CellResolution)
    {
        std::cout << resolution << "... ";
        float InvResolution = 1.0f / static_cast<float>(resolution);
        float3 dv = (m_SceneBox.max - m_SceneBox.min) / static_cast<float>(resolution);
        CellData current_cell = { 0, 0 };
        
        lower_indices = indices;
        lower_cells = cells;
        indices.clear();
        cells.clear();

        int totalCells = resolution * resolution * resolution;
        for (size_t z = 0; z < resolution; ++z)
        for (size_t y = 0; y < resolution; ++y)
        for (size_t x = 0; x < resolution; ++x)
        {
            Aabb curr_box(m_SceneBox.min + float3(x*dv.x, y*dv.y, z*dv.z), m_SceneBox.min + float3((x + 1)*dv.x, (y + 1)*dv.y, (z + 1)*dv.z));

            current_cell.start_index = indices.size();

            if (resolution == 2)
            {
                for (size_t i = 0; i < triangles.size(); ++i)
                {
                    if (curr_box.Intersects(triangles[i]))
                    {
                        indices.push_back(i);
                    }
                }
            }
            else
            {
                CellData lower_cell = lower_cells[x / 2 + (y / 2) * (resolution / 2) + (z / 2) * (resolution / 2) * (resolution / 2)];
                for (int i = lower_cell.start_index; i < lower_cell.start_index + lower_cell.count; ++i)
                {
                    if (curr_box.Intersects(triangles[lower_indices[i]]))
                    {
                        indices.push_back(lower_indices[i]);
                    }
                }
            }
            
            current_cell.count = indices.size() - current_cell.start_index;
            cells.push_back(current_cell);
            
        }
        resolution *= 2;

    }
    
    std::cout << "Done (" << (clock() - t) << " ms elapsed)" << std::endl;

}

#include <cctype>

class FileReader
{
public:
    FileReader(const char* filename) : m_InputFile(filename), m_VectorValue(0.0f)
    {
        if (!m_InputFile)
        {
            std::cerr << "Failed to load " << filename << std::endl;
        }
    }

    std::string GetStringValue() const
    {
        return m_StringValue;
    }

    float3 GetVectorValue() const
    {
        return m_VectorValue;
    }

protected:
    float ReadFloatValue()
    {
        SkipSpaces();
        float value = 0.0f;
        bool minus = m_CurrentLine[m_CurrentChar] == '-';
        if (minus) ++m_CurrentChar;
        while (m_CurrentChar < m_CurrentLine.size() && m_CurrentLine[m_CurrentChar] >= '0' && m_CurrentLine[m_CurrentChar] <= '9')
        {
            value = value * 10.0f + (float)((int)m_CurrentLine[m_CurrentChar++] - 48);
        }
        if (m_CurrentLine[m_CurrentChar++] == '.')
        {
            size_t frac = 1;
            while (m_CurrentChar < m_CurrentLine.size() && m_CurrentLine[m_CurrentChar] >= '0' && m_CurrentLine[m_CurrentChar] <= '9')
            {
                value += (float)((int)m_CurrentLine[m_CurrentChar++] - 48) / (pow(10, frac++));
            }
        }

        return minus ? -value : value;
        
    }

    int ReadIntValue()
    {
        SkipSpaces();
        int value = 0;
        bool minus = m_CurrentLine[m_CurrentChar] == '-';
        if (minus) ++m_CurrentChar;

        while (m_CurrentChar < m_CurrentLine.size() && m_CurrentLine[m_CurrentChar] >= '0' && m_CurrentLine[m_CurrentChar] <= '9')
        {
            value = value * 10 + ((int)m_CurrentLine[m_CurrentChar++] - 48);
        }
        return minus ? -value : value;
    }

    void ReadVectorValue()
    {
        m_VectorValue.x = ReadFloatValue();
        m_VectorValue.y = ReadFloatValue();
        m_VectorValue.z = ReadFloatValue();

    }

    void ReadStringValue()
    {
        SkipSpaces();
        m_StringValue.clear();
        while (m_CurrentChar < m_CurrentLine.size() && (m_CurrentLine[m_CurrentChar] >= 'a' && m_CurrentLine[m_CurrentChar] <= 'z' ||
            m_CurrentLine[m_CurrentChar] >= 'A' && m_CurrentLine[m_CurrentChar] <= 'Z'))
        {
            m_StringValue += m_CurrentLine[m_CurrentChar++];
        }
    }

    bool ReadLine()
    {
        do
        {
            if (!std::getline(m_InputFile, m_CurrentLine))
            {
                return false;
            }
        } while (m_CurrentLine.size() == 0);

        m_CurrentChar = 0;
        return true;
    }

    void SkipSpaces()
    {
        while (std::isspace(m_CurrentLine[m_CurrentChar])) { m_CurrentChar++; }
    }

    void SkipSymbol(char symbol)
    {
        SkipSpaces();
        while (m_CurrentLine[m_CurrentChar] == symbol) { m_CurrentChar++; }
    }

private:
    std::ifstream m_InputFile;
    std::string m_CurrentLine;
    std::string m_StringValue;
    float3 m_VectorValue;
    size_t m_CurrentChar;

};

class MtlReader : public FileReader
{
public:
    enum MtlToken_t
    {
        MTL_MTLNAME,
        MTL_DIFFUSE,
        MTL_SPECULAR,
        MTL_EMISSION,
        MTL_INVALID,
        MTL_EOF
    };

    MtlReader(const char* filename) : FileReader(filename)
    {
        std::cout << "Loading material library " << filename << std::endl;
    }

    MtlToken_t NextToken()
    {
        if (!ReadLine())
        {
            return MTL_EOF;
        }
        
        ReadStringValue();
        if (GetStringValue() == "newmtl")
        {
            ReadStringValue();
            return MTL_MTLNAME;
        }
        else if (GetStringValue() == "Kd")
        {
            ReadVectorValue();
            return MTL_DIFFUSE;
        }
        else if (GetStringValue() == "Ke")
        {
            ReadVectorValue();
            return MTL_EMISSION;
        }
        else if (GetStringValue() == "Ns")
        {
            ReadVectorValue();
            return MTL_SPECULAR;
        }
        else
        {
            return MTL_INVALID;
        }

    }
    
};


void Scene::LoadMtlFile(const char* filename)
{
    MtlReader mtlReader(filename);

    MtlReader::MtlToken_t token;
    Material currentMaterial;
    std::string materialName;
    while ((token = mtlReader.NextToken() ) != MtlReader::MTL_EOF)
    {
        switch (token)
        {
        case MtlReader::MTL_MTLNAME:
            materialName = mtlReader.GetStringValue();
            break;
        case MtlReader::MTL_DIFFUSE:
            currentMaterial.diffuse = mtlReader.GetVectorValue();
            std::cout << materialName << " diffuse " << currentMaterial.diffuse << std::endl;
            break;
        case MtlReader::MTL_SPECULAR:
            currentMaterial.specular = mtlReader.GetVectorValue();
            std::cout << materialName << " specular " << currentMaterial.specular << std::endl;
            break;
        case MtlReader::MTL_EMISSION:
            currentMaterial.emission = mtlReader.GetVectorValue();
            std::cout << materialName << " emission " << currentMaterial.emission << std::endl;
            materials[materialName] = currentMaterial;
            break;
        }
    }


}


class ObjReader : public FileReader
{
public:
    enum ObjToken_t
    {
        OBJ_MTLLIB,
        OBJ_USEMTL,
        OBJ_POSITION,
        OBJ_TEXCOORD,
        OBJ_NORMAL,
        OBJ_FACE,
        OBJ_INVALID,
        OBJ_EOF
    };

    ObjReader(const char* filename) : FileReader(filename)
    {
        std::cout << "Loading object file " << filename << std::endl;
        
    }

    ObjToken_t NextToken()
    {
        if (!ReadLine())
        {
            return OBJ_EOF;
        }

        ReadStringValue();
        if (GetStringValue() == "mtllib")
        {
            ReadStringValue();
            return OBJ_MTLLIB;
        }
        else if (GetStringValue() == "usemtl")
        {
            ReadStringValue();
            return OBJ_USEMTL;
        }
        else if (GetStringValue() == "v")
        {
            ReadVectorValue();
            return OBJ_POSITION;
        }
        else if (GetStringValue() == "vn")
        {
            ReadVectorValue();
            return OBJ_NORMAL;
        }
        else if (GetStringValue() == "f")
        {
            for (size_t i = 0; i < 3; ++i)
            {
                m_VertexIndices[i] = ReadIntValue() - 1;
                SkipSymbol('/');
                m_TexcoordIndices[i] = ReadIntValue() - 1;
                SkipSymbol('/');
                m_NormalIndices[i] = ReadIntValue() - 1;

            }
            return OBJ_FACE;
        }
        else
        {
            return OBJ_INVALID;
        }
                
    }
    
    void GetFaceIndices(int** iv, int** it, int** in)
    {
        *iv = m_VertexIndices;
        *it = m_TexcoordIndices;
        *in = m_NormalIndices;

    }

private:
    // Face indices
    int m_VertexIndices[3];
    int m_TexcoordIndices[3];
    int m_NormalIndices[3];

};

void Scene::LoadTriangles()
{
    ObjReader objReader(m_Filename);
    
    std::vector<float3> vertices;
    std::vector<float3> normals;

    Material currentMaterial;
    std::string materialName;
    ObjReader::ObjToken_t token;


    while ((token = objReader.NextToken()) != ObjReader::OBJ_EOF)
    {
        switch (token)
        {
        case ObjReader::OBJ_MTLLIB:
            LoadMtlFile(("meshes/" + objReader.GetStringValue() + ".mtl").c_str());
            break;
        case ObjReader::OBJ_POSITION:
            vertices.push_back(objReader.GetVectorValue());
            for (int i = 0; i < 3; ++i)
            {
                m_SceneBox.min[i] = std::min(objReader.GetVectorValue().x, m_SceneBox.min[i]);
                m_SceneBox.max[i] = std::max(objReader.GetVectorValue().x, m_SceneBox.max[i]);
            }

            break;
        case ObjReader::OBJ_NORMAL:
            normals.push_back(objReader.GetVectorValue());
            break;
        case ObjReader::OBJ_USEMTL:
            currentMaterial = materials[objReader.GetStringValue()];
            break;
        case ObjReader::OBJ_FACE:
            int *iv, *it, *in;
            objReader.GetFaceIndices(&iv, &it, &in);
            triangles.push_back(Triangle(vertices[iv[0]], vertices[iv[1]], vertices[iv[2]], normals[in[0]], normals[in[1]], normals[in[2]], currentMaterial));
            break;
        }
    }

    for (int i = 0; i < 3; ++i)
    {
        m_SceneBox.max[i] = 64;// pow(2, (int)log2(abs(m_SceneBox.max[i])) + 1) * ((m_SceneBox.max[i] > 0) ? 1 : ((m_SceneBox.max[i] < 0) ? -1 : 0));
        m_SceneBox.min[i] = 0;// pow(2, (int)log2(abs(m_SceneBox.min[i])) + 1) * ((m_SceneBox.min[i] > 0) ? 1 : ((m_SceneBox.min[i] < 0) ? -1 : 0));
    }
    
    std::cout << "Load succesful (" << triangles.size() << " triangles)" << std::endl;

}
