#pragma once
#include "ArgsBuilder/Builder.hpp"
#include "Logger/Log.hpp"
#include <vector>
#include <unordered_map>
namespace d3156
{
    namespace PluginCore
    {
        typedef std::string model_name;
        class IModel
        {
        public:
            virtual ~IModel() {}

            // Модель должна создаваться в init. Конструктор должен быть пустой
            virtual model_name name() = 0;

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

#define RegisterModel(model_name, new_model, T)                                                                        \
    ((models.contains(model_name)) ? models.getModel<T>(model_name) : models.registerModel(new_model))

        /// \brief Хранилище моделей данных
        class ModelsStorage : protected std::unordered_map<model_name, IModel *>
        {
        public:
            /// \attention Если модель с таким же именем есть, переданная модель будет удалена, а
            /// функция вернет мадель из хранилища. Любое взаимодействие с моделью должно производиться ТОЛЬКО ПОСЛЕ
            /// РЕГИСТРАЦИИ, иначе все изменения могут быть утеряны
            template <class ConcreteModel> ConcreteModel *registerModel(ConcreteModel *model)
            {
                auto it = find(model->name());
                if (it != end()) {
                    delete model;
                    
                    Y_LOG(0, "Model already registred :" << model->name().c_str() << "\n");
                    return static_cast<ConcreteModel *>(it->second);
                } else {
                    model->init();
                    insert({model->name(), model});
                    regSeq_.push_back(model);
                    G_LOG(0, "Model registred success [Delete order " << model->deleteOrder() << "] " << model->name().c_str() << "\n");
                    return model;
                }
            }

            template <class ConcreteModel> ConcreteModel *getModel(model_name name)
            {
                auto it = find(name);
                if (it != end()) return static_cast<ConcreteModel *>(it->second);
                return nullptr;
            }

            bool contains(model_name name) { return find(name) != end(); }

            void reset();

            ~ModelsStorage();

        private:
            std::vector<IModel *> regSeq_;
            friend class Core;
        };
    }
}