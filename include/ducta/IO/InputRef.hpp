/**
 * @file InputRef.hpp
 * @brief Type-erased input wrapper for handling input values
 */

#ifndef DUCTA_T_INPUT_REF_HPP
#define DUCTA_T_INPUT_REF_HPP

#include "ducta/Core/Types/TypeIndex.hpp"
#include "ducta/Core/Types/Map.hpp"
#include "ducta/Core/Types/StringView.hpp"

namespace ducta {
namespace IO {

/**
 * @brief Class that represents a type-erased reference of input. Read-only.
 * 
 */
class InputRef
{
private:
    struct VTable {
        bool (*is_ready)(void*);
        bool (*is_compatible)(void*, TypeIndex);
        void (*notify)(void*, TypeIndex, void const*);
        TypeIndex (*type_id)(void*);
    };

    template<class T>
    static const VTable& vt_for()
    {
        static const VTable vt = {
            +[](void* obj) -> bool { 
                return static_cast<T*>(obj)->ready(); 
            },
            +[](void* obj, TypeIndex type) -> bool { 
                return static_cast<T*>(obj)->compatible(type); 
            },
            +[](void* obj, TypeIndex type, void const* value) { 
                using ValueType = typename T::value_type;
                if (type != ::ducta::type_id<ValueType>())
                {
                    throw std::runtime_error("Incompatible type for notification");
                }

                static_cast<T*>(obj)->notify(*static_cast<ValueType const*>(value));
            },
            +[](void* obj) -> TypeIndex {
                return ::ducta::type_id<T>(); 
            }
        };
        return vt;
    }

public:
    /**
     * @brief Construct InputRef from a concrete input type
     * @tparam TInputType The concrete input type
     * @param input The input object to wrap
     */
    template <typename TInputType,
              typename = ::ducta::enable_if_t<!::ducta::is_same_v<::ducta::decay_t<TInputType>, InputRef>>>
    explicit InputRef(TInputType& input)
        : m_input_object(&input)
        , m_vtable(&vt_for<::ducta::decay_t<TInputType>>())
    {}

    /**
     * @brief Check if input has a ready value
     * @return true if value is ready, false otherwise
     */
    bool ready() const
    {
        return m_vtable->is_ready(m_input_object);
    }

    /**
     * @brief Check if the input is compatible with the given type
     * @param type The type to check compatibility against
     * @return true if compatible, false otherwise
     */
    bool compatible(TypeIndex type) const
    {
        return m_vtable->is_compatible(m_input_object, type);
    }

    /**
     * @brief Notify the input of a new value
     * @tparam T The type of the value
     * @param value The value to notify
     */
    template <typename T>
    void notify(T const& value)
    {
        m_vtable->notify(m_input_object, type_id<::ducta::decay_t<T>>(), &value);
    }

    /**
     * @brief Attempt to cast the InputRef back to its original concrete type
     * @tparam InputT The concrete input type to cast to
     * @return Pointer to the concrete input type if successful, nullptr otherwise
     */
    template <typename InputT>
    InputT* as()
    {
        if (type_id<InputT>() != m_vtable->type_id(m_input_object))
            return nullptr;

        return static_cast<::ducta::decay_t<InputT>*>(m_input_object);
    }

    /**
    * @brief Friend function to cast InputRef to concrete type
    * @tparam InputT The concrete input type to cast to
    * @param input The InputRef to cast
    * @return Pointer to the concrete input type if successful, nullptr otherwise
    */
    template <typename InputT>
    friend InputT* cast(InputRef input);

    /**
     * @brief Equality operator for InputRef
     * @param lhs Left-hand side InputRef
     * @param rhs Right-hand side InputRef
     * @return true if both InputRefs refer to the same underlying input, false otherwise
     */
    friend bool operator==(InputRef const& lhs, InputRef const& rhs)
    {
        if(lhs.m_vtable != rhs.m_vtable)
            return false;
        return lhs.m_input_object == rhs.m_input_object;
    }

private:
    void* m_input_object;
    const VTable* m_vtable;
};

template <typename InputT>
InputT* cast(InputRef input)
{
    return input.as<InputT>();
}

template <typename Node>
struct NodeInputsTraits
{
    static Map<StringView, IO::InputRef, 25> get(Node& node)
    {
        return {};
    }
};

} // namespace IO
} // namespace ducta

#endif // DUCTA_T_INPUT_REF_HPP