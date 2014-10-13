#pragma once
#include "Node.hpp"
#include <iomanip>

namespace opencl
{

namespace JIT
{

    class UnaryNode : public Node
    {
    private:
        std::string m_op_str;
        Node *m_child;
        int m_op;

    public:
        UnaryNode(const char *out_type_str,
                  const char *op_str,
                  Node *child, int op)
            : Node(out_type_str),
              m_op_str(op_str),
              m_child(child),
              m_op(op)
        {
        }

        void genParams(std::stringstream &kerStream)
        {
            if (m_gen_param) return;
            if (!(m_child->isGenParam())) m_child->genParams(kerStream);
            m_gen_param = true;
        }

        int setArgs(cl::Kernel &ker, int id)
        {
            return m_child->setArgs(ker, id);
        }

        void genOffsets(std::stringstream &kerStream)
        {
            if (m_gen_offset) return;
            if (!(m_child->isGenOffset())) m_child->genOffsets(kerStream);
            m_gen_offset = true;
        }

        void genKerName(std::stringstream &kerStream, bool genInputs)
        {
            if (!genInputs) {
                // Make the hex representation of enum part of the Kernel name
                kerStream << std::setw(2) << std::setfill('0') << std::hex << m_op << std::dec;
            }
            m_child->genKerName(kerStream, genInputs);
        }

        void genFuncs(std::stringstream &kerStream)
        {
            if (m_gen_func) return;

            if (!(m_child->isGenFunc())) m_child->genFuncs(kerStream);

            kerStream << m_type_str << " val" << m_id << " = "
                      << m_op_str << "(val" << m_child->getId() << ");"
                      << std::endl;

            m_gen_func = true;
        }

        int setId(int id)
        {
            if (m_set_id) return id;

            id = m_child->setId(id);

            m_id = id;
            m_set_id = true;

            return m_id + 1;
        }

    };

}

}