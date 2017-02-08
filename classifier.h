#pragma once
#include "classifier.h"
#include <caffe/caffe.hpp>
#include <caffe/layers/memory_data_layer.hpp>
#include <memory>
#ifdef USE_OPENCV
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#endif  // USE_OPENCV
#include <experimental/filesystem>
#include <iostream>

using namespace caffe;
namespace fs = std::experimental::filesystem;

namespace qflow {
namespace video {
class classifier
{
public:
    classifier(std::string model_folder, std::string mean_file, std::string label_file, uint batch_size = 100) : model_folder_(model_folder), mean_file_(mean_file), label_file_(label_file), batch_size_(batch_size)
    {
        Caffe::set_mode(Caffe::GPU);
        Caffe::SetDevice(0);
        model_file_ = model_folder_ / "deploy.prototxt";
        fs::file_time_type latest;
        for(auto& p: fs::directory_iterator(model_folder_))
        {
            auto time = fs::last_write_time(p);
            if(p.path().extension() == ".caffemodel" && time > latest)
            {
                latest = time;
                trained_file_ = p;
            }
        }
        net_.reset(new Net<float>(model_file_.string(), TEST));
        net_->CopyTrainedLayersFrom(trained_file_.string());
        boost::shared_ptr<Blob<float>> input_layer= net_->blobs()[0];
        num_channels_ = input_layer->channels();
        input_geometry_ = cv::Size(input_layer->width(), input_layer->height());
        set_mean();
        std::ifstream labels(label_file_.string());
        std::string line;
        while (std::getline(labels, line))
            labels_.push_back(std::string(line));
        Blob<float>* output_layer = net_->output_blobs()[0];
        CHECK_EQ(labels_.size(), output_layer->channels())
                << "Number of labels is different from the output layer dimension.";
        if(batch_size_ != -1)
        {
            input_layer->Reshape(batch_size_, num_channels_,
                                 input_geometry_.height, input_geometry_.width);
            net_->Reshape();
        }
        else
        {
            batch_size = input_layer->count();
        }
    }
    bool append(AVFramePointer frame)
    {
        Net<float>* n =  net_.get();
        cv::Mat sample(frame->height, frame->width, CV_8UC3, (void*)frame->data[0]);
        /*boost::shared_ptr<MemoryDataLayer<float>> md_layer =
            boost::dynamic_pointer_cast <MemoryDataLayer<float>>(d_ptr->_net->layers()[0]);
        std::vector<cv::Mat> images(1, sample);
        std::vector<int> labels(1, 0);
        md_layer->AddMatVector(images, labels);
        d_ptr->_net->Forward();
        shared_ptr<Blob<float>> prob = d_ptr->_net->blob_by_name("prob");
        const float* begin = prob->cpu_data();
        const float* end = begin + prob->channels();*/

        boost::shared_ptr<Blob<float>> input_layer= net_->blobs()[0];

        cv::Mat sample_float;
        sample.convertTo(sample_float, CV_32FC3);
        cv::Mat sample_normalized;
        cv::subtract(sample_float, mean_, sample_normalized);
        cv::Mat sample_final;
        cv::resize(sample_normalized, sample_final, cv::Size(input_geometry_.width, input_geometry_.height));


        std::vector<cv::Mat> input_channels;
        int inputSize = input_geometry_.width * input_geometry_.height;
        float* input_data = input_layer->mutable_cpu_data() +  counter_ * inputSize * input_layer->channels();
        for (int i = 0; i < input_layer->channels(); ++i) {
            cv::Mat channel(input_geometry_.height, input_geometry_.width, CV_32FC1, input_data);
            input_channels.push_back(channel);
            input_data += inputSize;
        }
        cv::split(sample_final, input_channels);
        counter_++;
        return counter_ == batch_size_;
    }
    auto forward()
    {
        net_->Forward();

        boost::shared_ptr<Blob<float>> output_layer = net_->blobs()[net_->blobs().size()-1];
        int t=output_layer->channels();
        int y=output_layer->count();
        std::vector<std::vector<float>> table;
        for(int i=0; i<counter_; i++)
        {
            std::vector<float> record;
            const float* begin = output_layer->cpu_data() + i*output_layer->channels();
            const float* end = begin + output_layer->channels();
            std::vector<float> predictions(begin, end);
            for(float prediction: predictions)
            {
                record.push_back(prediction);
            }
            std::vector<float>::iterator result = std::max_element(predictions.begin(), predictions.end());
            int index = std::distance(predictions.begin(), result);
            std::cout << "Winner at: " << index << " value: " << *result << " label: " << labels_[index];
            table.push_back(record);
        }
        counter_ = 0;
        return table;
    }
    std::vector<std::string> labels() const
    {
        return labels_;
    }

private:
    cv::Size input_geometry_;
    uint num_channels_;
    uint batch_size_;
    cv::Mat mean_;
    fs::path model_folder_;
    fs::path model_file_;
    fs::path trained_file_;
    fs::path mean_file_;
    fs::path label_file_;
    std::unique_ptr<Net<float>> net_;
    std::vector<std::string> labels_;
    int counter_ = 0;

    void set_mean() {
        BlobProto blob_proto;
        ReadProtoFromBinaryFileOrDie(mean_file_.string(), &blob_proto);
        Blob<float> mean_blob;
        mean_blob.FromProto(blob_proto);
        std::vector<cv::Mat> channels;
        float* data = mean_blob.mutable_cpu_data();
        for (int i = 0; i < num_channels_; ++i) {
            cv::Mat channel(mean_blob.height(), mean_blob.width(), CV_32FC1, data);
            channels.push_back(channel);
            data += mean_blob.height() * mean_blob.width();
        }
        cv::merge(channels, mean_);
    }

};
}
std::string to_string(const std::vector<float>& row)  
{  
    std::string str;
    for(int i=0; i<row.size() - 1; i++)
    {
        str += std::to_string(row[i]) + ',';
    }
    str += std::to_string(row.back());
    return str;
}
std::string to_string(const std::vector<std::vector<float>>& table)  
{  
    std::string str;
    for(int i=0; i<table.size(); i++)
    {
        str += qflow::to_string(table[i]) + '\n';
    }
    return str;
}
}
std::ostream& operator<<(std::ostream& os, const std::vector<string>& header)  
{  
    for(int i=0; i<header.size() - 1; i++)
    {
        os << header[i] << ',';
    }
    os << header.back() << '\n';
    return os; 
}
std::ostream& operator<<(std::ostream& os, const std::vector<float>& row)  
{  
    os << qflow::to_string(row);
    return os; 
}
std::ostream& operator<<(std::ostream& os, const std::vector<std::vector<float>>& table)  
{  
    os << qflow::to_string(table);
    return os;  
}
