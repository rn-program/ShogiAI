#include "nn/nn_model.hpp"
#include <iostream>

NNModel::NNModel(const std::string &model_path, torch::Device device)
    : device_(device)
{
    module_ = torch::jit::load(model_path, device_);
    module_.eval();

    std::cout << "[NNModel] Loaded: " << model_path << std::endl;
}

std::pair<torch::Tensor, torch::Tensor>
NNModel::forward(const torch::Tensor &input)
{
    // forward は IValue の vector を受け取る
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(input.to(device_));

    auto output = module_.forward(inputs).toTuple();

    torch::Tensor policy = output->elements()[0].toTensor();
    torch::Tensor value = output->elements()[1].toTensor();

    return {policy, value};
}
