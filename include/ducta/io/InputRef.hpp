/**
 * @file TInput.hpp
 * @brief Type-erased input wrapper for handling input values
 */

#ifndef DUCTA_T_INPUT_REF_HPP
#define DUCTA_T_INPUT_REF_HPP

#include "ducta/IO/Private/InputModelRef.hpp"
#include "ducta/Utils/TypeIndex.hpp"

namespace ducta {
namespace IO {

/**
 * @brief Class that represents an type-erased reference of input. Read-only.
 * 
 */
class InputRef
{
public:
    /**
     * @brief Construct TInput from a concrete input type
     * @tparam TInputType The concrete input type
     * @param input The input object to wrap
     */
    template <typename TInputType,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<TInputType>, InputRef>>>
    explicit InputRef(TInputType& input)
        : m_concept(std::make_shared<Private::InputModelRef<std::decay_t<TInputType>>>(input))
    {}

    InputRef(InputRef const&) = default;
    InputRef& operator=(InputRef const&) = default;

    /**
     * @brief Check if input has a ready value
     * @return true if value is ready, false otherwise
     */
    bool ready() const
    {
        return m_concept->is_ready();
    }

    bool compatible(Utils::TypeIndex type) const
    {
        return m_concept->is_compatible(type);
    }

    template <typename T>
    void notify(T const& value)
    {
        m_concept->notify(Utils::type_id<std::decay_t<T>>(), &value);
    }

    template <typename InputT>
    InputT* as()
    {
        if (Utils::type_id<InputT>() != m_concept->type_id())
            return nullptr;

        using ModelRefT = Private::InputModelRef<InputT>;
        auto* model = static_cast<ModelRefT*>(m_concept.get());
        return &model->get_input();
    }


    template <typename InputT>
    friend InputT* cast(InputRef input);

    friend bool operator==(InputRef const& lhs, InputRef const& rhs)
    {
        return lhs.m_concept->areEqual(*rhs.m_concept);
    }

private:
    std::shared_ptr<Private::InputConcept> m_concept; ///< Type-erased input implementation
};

template <typename InputT>
InputT* cast(InputRef input)
{
    return input.as<InputT>();
}

template <typename Node>
struct NodeInputsTraits
{
    static std::map<std::string, IO::InputRef> get(Node& node)
    {
        return std::map<std::string, IO::InputRef>{};
    }
};

} // namespace IO
} // namespace ducta

#endif // DUCTA_T_INPUT_REF_HPP