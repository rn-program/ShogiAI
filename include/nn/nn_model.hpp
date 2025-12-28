#pragma once

#include <torch/script.h>
#include <string>
#include <utility>

class NNModel
{
public:
    // コンストラクタでロード
    explicit NNModel(
        const std::string &model_path,
        torch::Device device = torch::kCPU);

    // 推論
    // 戻り値: (policy, value)
    std::pair<torch::Tensor, torch::Tensor>
    forward(const torch::Tensor &input);

private:
    torch::jit::script::Module module_;
    torch::Device device_;
};
