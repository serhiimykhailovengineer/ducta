#ifndef DUCTA_PIPELINE_STOPTOKEN_HPP
#define DUCTA_PIPELINE_STOPTOKEN_HPP

#include "ducta/Core/TypeTraits.hpp"

namespace ducta {
namespace Pipeline {
class StopToken
{
private:
    struct VTable {
        bool (*stopRequested)(void const*) ;
    };

    template<class T>
    static const VTable& vt_for()
    {
        static const VTable vt = {
            +[](void const* obj) -> bool { return static_cast<T const*>(obj)->stopRequested(); },
        };
        return vt;
    }
public:
template <typename StopTokenType,
              typename = ::ducta::enable_if_t<!::ducta::is_same_v<::ducta::decay_t<StopTokenType>, StopToken>>>
    StopToken(StopTokenType const& stopToken)
    : m_stop_token_obj{&stopToken}
    , m_vtable{&vt_for<StopTokenType>()}
    {
    }

    StopToken(StopToken const& other) = default;
    StopToken& operator=(StopToken const& other) = default;

    StopToken(StopToken&& other) noexcept = default;
    StopToken& operator=(StopToken&& other) noexcept = default;

    bool stopRequested() const
    {
        return m_vtable->stopRequested(m_stop_token_obj);
    }

private:
    void const* m_stop_token_obj;
    const VTable* m_vtable;
};
} // namespace Pipeline
} // namespace ducta

#endif // DUCTA_PIPELINE_STOPTOKEN_HPP