/**
 * @file OutputRef.hpp
 * @brief Type-erased output wrapper for handling output values
 */

#ifndef DUCTA_IO_OUTPUT_REF_HPP
#define DUCTA_IO_OUTPUT_REF_HPP

#include "ducta/IO/InputRef.hpp"

#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/StringView.hpp"

#include "ducta/IO/Connection.hpp"

namespace ducta {
namespace IO {

/**
 * @brief Class that represents a type-erased reference of output.
 * 
 */
class OutputRef
{
private:
    struct VTable {
        Connection (*do_bind)(void*, InputRef&);
        TypeIndex (*type_id)(void*);
    };

    template<class T>
    static const VTable& vt_for()
    {
        static const VTable vt = {
            +[](void* obj, InputRef& input) -> Connection {
                return static_cast<T*>(obj)->bind(input); 
            },
            +[](void* obj) -> TypeIndex {
                return ::ducta::type_id<T>(); 
            }
        };
        return vt;
    }

public:
    /**
     * @brief Construct OutputRef from a concrete output type
     * @tparam TOutputType The concrete output type
     * @param output The output object to wrap
     */
    template <typename TOutputType,
              typename = ::ducta::enable_if_t<!::ducta::is_same_v<::ducta::decay_t<TOutputType>, OutputRef>>>
    explicit OutputRef(TOutputType& output)
    : m_output_object{&output}
    , m_vtable{&vt_for<::ducta::decay_t<TOutputType>>()}
    {}

    /**
     * @brief Bind the output to an input
     * @param input The input to bind to
     * @return A Connection object that will unbind the input when destroyed
     */
    Connection do_bind(InputRef input)
    {
        return m_vtable->do_bind(m_output_object, input);
    }

    /**
     * @brief Equality operator for OutputRef
     * @param lhs Left-hand side OutputRef
     * @param rhs Right-hand side OutputRef
     * @return true if both OutputRefs refer to the same underlying output, false otherwise
     */
    friend bool operator==(OutputRef const& lhs, OutputRef const& rhs)
    {
        if(lhs.m_vtable != rhs.m_vtable)
            return false;
        return lhs.m_output_object == rhs.m_output_object;
    }

private:
    void* m_output_object;
    const VTable* m_vtable;
};

inline Connection bind(OutputRef output, InputRef input)
{
    return output.do_bind(input);
}

template <typename Node>
struct NodeOutputsTraits
{
    static Map<StringView, IO::OutputRef, 25> get(Node& node)
    {
        return {};
    }
};

} // namespace IO
} // namespace ducta

#endif // DUCTA_IO_OUTPUT_REF_HPP