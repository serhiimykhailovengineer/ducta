#ifndef DUCTA_T_OUTPUT_HPP
#define DUCTA_T_OUTPUT_HPP

namespace ducta {
namespace IO {

template <typename T>
class TOutput
{
private:
    class Concept
    {
    public:
        virtual ~Concept() = default;

        virtual void set(T const& value) = 0;
        virtual void set(T&& value) = 0;

        virtual OutputRef get_ref() = 0;
    };

    template <typename OutputT>
    class Model : public Concept
    {
        OutputT m_output;

    public:
        Model(OutputT output)
            : m_output(std::move(output))
        {
        }

        void set(T const& value) override
        {
            m_output.set(value);
        }

        void set(T&& value) override
        {
            m_output.set(std::move(value));
        }

        OutputRef get_ref() override
        {
            return OutputRef{m_output};
        }
    };

public:
    template <typename TOutputType>
    TOutput(TOutputType&& output);

    void set(T const& value) { m_output->set(value); }
    void set(T&& value) { m_output->set(std::move(value)); }

    TOutput<T>& operator=(T const& value)
    {
        set(value);
        return *this;
    }

    TOutput<T>& operator=(T&& value)
    {
        set(std::move(value));
        return *this;
    }

    OutputRef get_ref()
    {
        return m_output->get_ref();
    }

private:
    std::unique_ptr<Concept> m_output;
};

template <typename T>
template <typename TOutputType>
TOutput<T>::TOutput(TOutputType&& output)
: m_output(std::make_unique<Model<std::decay_t<TOutputType>>>(std::forward<TOutputType>(output)))
{
}

} // namespace io
} // namespace ducta

#endif // DUCTA_T_OUTPUT_HPP