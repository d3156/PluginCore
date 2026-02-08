#pragma once
#include "ArgsBuilder/Builder.hpp"
#include "Logger/Log.hpp"
#include <set>
#include <unordered_map>

namespace d3156::PluginCore
{

    class IModel
    {
    public:
        virtual ~IModel() = default;

        /// \brief DeleteOrder Указать приоритет вызова деструктора модели
        /// \return Значение приоритета. Чем меньше число - тем раньше будет удалена модель
        /// (отрицательные числа поддерживаются)
        virtual int deleteOrder() { return 0; }

        // Создание всех объектов модели необходимо выполнять в методе init. Конструктор должен быть пустым.
        virtual void init() = 0;

        /// \brief postInit Вызывается после всех шагов инициализации плагинов
        virtual void postInit() {}

        /// \brief registerArgs Зарегистрировать аргументы командной строки
        /// \param bldr Анализатор командной строки
        /// \note Значения аргументов распарсятся до postInit
        virtual void registerArgs(Args::Builder &bldr) {}
    };

    /// \brief Хранилище моделей данных
    class ModelsStorage : protected std::unordered_map<std::string, IModel *>
    {
    public:
        /// \attention Если модель с таким уже именем есть, она будет возвращена из хранилища. Иначе - создана новая
        template <class ConcreteModel, typename... _Args> ConcreteModel *registerModel(_Args &&...__args)
        {
            auto it = find(ConcreteModel::name());
            plugins_req_model[ConcreteModel::name()].insert(current_plugin);
            if (it == end()) {
                auto model = new ConcreteModel(std::forward<_Args>(__args)...);
                model->init();
                orders.insert(model->deleteOrder());
                insert({ConcreteModel::name(), model});
                G_LOG(0, "Model registered success [Delete order " << model->deleteOrder() << "] "
                                                                   << ConcreteModel::name());
                return model;
            }
            G_LOG(0, "Model already registered :" << ConcreteModel::name() << "\n");
            return static_cast<ConcreteModel *>(it->second);
        }

        ~ModelsStorage();

    private:
        void reset();
        void finishRegistering();
        std::set<int> orders;
        std::string current_plugin;
        std::unordered_map<std::string, std::set<std::string>> plugins_req_model;
        friend class Core;
    };
}
