#pragma once
#include "structs.hpp"
#include <iostream>
#include <vector>

namespace PluginCore
{

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

    public:
        // Создание всех объектов модели необходимо выполнять в методе init. Конструктор должен быть пустым.
        virtual void init() = 0;

        /// \brief postInit Вызывается после всех шагов инициализации плагинов
        virtual void postInit() {}
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
                std::cout << Y_CORE << "Модель уже зарегистрирована :" << model->name().c_str();
                return static_cast<ConcreteModel *>(it->second);
            } else {
                model->init();
                insert({model->name(), model});
                regSeq_.push_back(model);
                std::cout << G_CORE << "Модель зарегистрирована успешно :"
                          << "[Приоритет удаления" << model->deleteOrder() << "]" << model->name().c_str();
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

        ~ModelsStorage();

    private:
        std::vector<IModel *> regSeq_;
        friend class Core;
    };
}