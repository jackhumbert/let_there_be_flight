// LTBF R41: jackhumbert-fork additions to RED4ext/Relocation.hpp, appended inside namespace RED4ext of the Mac SDK header by
// macos/CMakeLists.txt. The fork headers LTBF uses call game functions through IHookable::StaticHook and UniRelocFunc.

/**
 * @brief Calls a game virtual through a class vtable known by its hash, at a Windows slot offset (fork convention).
 *
 * macOS: the vtable hash and slot are translated by ltbf_mac_vft_slot (macos/MacShims.cpp): a table of the Mac functions behind
 * the slots LTBF's fork headers call (R40 evidence), or the trap stub for an unknown slot.
 */
extern "C" uintptr_t ltbf_mac_vft_slot(uint32_t aVftHash, uintptr_t aWindowsOffset);

template<typename T>
class UniRelocFunc
{
};

template<typename R, typename... Args>
class UniRelocFunc<R (*)(Args...)> : private UniversalRelocBase
{
public:
    using Fn = R (*)(Args...);
    UniRelocFunc(uint32_t aHash)
        : m_hash(aHash), m_slot(~uintptr_t(0)), m_address(nullptr)
    {
    }
    UniRelocFunc(uint32_t aVftHash, uintptr_t aWindowsOffset)
        : m_hash(aVftHash), m_slot(aWindowsOffset), m_address(nullptr)
    {
    }
    inline operator Fn() const
    {
        if (!m_address)
        {
            m_address = m_slot == ~uintptr_t(0) ? reinterpret_cast<Fn>(Resolve(m_hash))
                                                : reinterpret_cast<Fn>(ltbf_mac_vft_slot(m_hash, m_slot));
        }
        return m_address;
    }

private:
    uint32_t m_hash;
    uintptr_t m_slot;
    mutable Fn m_address;
};

template<typename C, typename R, typename... Args>
class UniRelocFunc<R (C::*)(Args...)> : public UniRelocFunc<R (*)(C*, Args...)>
{
    using UniRelocFunc<R (*)(C*, Args...)>::UniRelocFunc;
};

template<typename C, typename R, typename... Args>
class UniRelocFunc<R (C::*)(Args...) const> : public UniRelocFunc<R (*)(const C*, Args...)>
{
    using UniRelocFunc<R (*)(const C*, Args...)>::UniRelocFunc;
};

class IHookable
{
public:
    template<typename R, uint32_t Hash, typename... Args>
    inline R Hook(Args... args)
    {
        static auto func = UniversalRelocFunc<R (*)(IHookable*, Args...)>(Hash);
        return func(this, std::forward<Args>(args)...);
    }

    template<typename R, uint32_t Hash, typename... Args>
    inline R Hook(Args... args) const
    {
        static auto func = UniversalRelocFunc<R (*)(IHookable const*, Args...)>(Hash);
        return func(this, std::forward<Args>(args)...);
    }

    template<typename R, uint32_t Hash, typename... Args>
    static inline R StaticHook(Args... args)
    {
        static auto func = UniversalRelocFunc<R (*)(Args...)>(Hash);
        return func(std::forward<Args>(args)...);
    }
};
