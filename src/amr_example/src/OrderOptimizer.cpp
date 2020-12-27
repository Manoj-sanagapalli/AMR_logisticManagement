#include <iostream>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/point.hpp"
#include "custom_msg/msg/order_desc.hpp"
#include "visualization_msgs/msg/marker_array.hpp"
#include "visualization_msgs/msg/marker.hpp"
#include <sstream>
#include <fstream>
#include "yaml-cpp/yaml.h"
#include <string>
#include <experimental/filesystem>
#include <limits>
#include "rclcpp/time.hpp"

using namespace std::chrono_literals;
visualization_msgs::msg::MarkerArray markerArray;
visualization_msgs::msg::Marker marker;

uint32_t shape = visualization_msgs::msg::Marker::CUBE;

auto offset = std::chrono::seconds(1);
float MAX = std::numeric_limits<float>::max();
namespace fs = std::experimental::filesystem;

uint32_t orderId = 0;
float source_cx = 0, source_cy = 0;
std::string order_description;
const YAML::Node Load;
int tag = 0;

struct Product {
    float productId;
    std::string partName;
    float Cx;
    float Cy;
    int tag;
};
std::vector<Product> Fetch;
std::vector<size_t> Path;
struct Order {
    /* data */
    float Id;
    float cx;
    float cy;
    std::vector<float> products;
};

struct Parts {
    std::string partName;
    float Pcx;
    float Pcy;
};

struct Configuration {
    float product_Id;
    std::string pdtName;
    std::vector<Parts> pdtParts;
};

void parse(uint32_t id);
void getParts(float productId);
void output(std::vector<Product> Fetch, float cx, float cy);
void publish_marker(std::vector<Product> Fetch);

static custom_msg::msg::OrderDesc get_order;
std::string filepath;
std::string default_value = "/home/roscon";
using namespace std::chrono_literals;

class Order_optimizer : public rclcpp::Node {
public:
    Order_optimizer()
        : Node("Order_optimizer")
        , count_(0)
    {
        publishe_array = this->create_publisher<visualization_msgs::msg::MarkerArray>("order_path", 1);
        //Parameter declaration, assigning defaultvalue, reading from commandline
        this->declare_parameter("absolute_path", default_value);
        if (this->has_parameter("absolute_path")) {
            //RCLCPP_INFO(this->get_logger(), "parameter exists");
        }
        this->get_parameter("absolute_path", filepath);

        subscribe_position = this->create_subscription<geometry_msgs::msg::PoseStamped>(
            "currentPosition", 1,
            [this](geometry_msgs::msg::PoseStamped::SharedPtr get_currentPosition) {
                source_cx = get_currentPosition->pose.position.x;
                source_cy = get_currentPosition->pose.position.y;
            }
            );

        subscribe_order = this->create_subscription<custom_msg::msg::OrderDesc>(
            "nextOrder", 1,
            [this](custom_msg::msg::OrderDesc::SharedPtr get_orderId) {
                orderId = get_orderId->order_id;
                order_description = get_orderId->description;
            });
    }

    void parse(uint32_t orderId)
    {
        std::vector<std::string> file;
        //get all the files in the given folder
        for (const auto& entry : fs::directory_iterator(filepath + "/orders"))
            file.push_back(entry.path());
        //Itereate through each and every file inside the given absolute location
        size_t p = 0;
        bool orderFound = false;
        while ((p < file.size()) && !(orderFound)) {
            std::cout << file[p] << '\n';
            //Input the file to istream
            std::ifstream fin(file[p], std::fstream::in);
            // Input the yaml file as Node object
            YAML::Node node = YAML::Load(fin);
            //std::cout<<"the size of the doc is \t"<<node.size()<<std::endl;
            //Iterate through document
            for (YAML::iterator iterator = node.begin(); iterator != node.end(); ++iterator) {
                YAML::Node orderNode = *iterator;

                if (orderNode["order"].as<uint32_t>() == orderId) {
                    float Destination_x = orderNode["cx"].as<float>();
                    float Destination_y = orderNode["cy"].as<float>();
                    orderFound = true;
                    //Iterate through products
                    for (size_t j = 0; j < orderNode["products"].size(); ++j) {

                        getParts(orderNode["products"][j].as<float>());
                    }
                    std::cout << "the number of parts are" << Fetch.size() << '\n';
                    output(Fetch, Destination_x, Destination_y);
                    publish_marker(Fetch);
                    //publishMarkers(Fetch);
                    Fetch.clear();
                }
            }

            p++;
        }
    }

    void getParts(float id)
    {
        Product product;
        std::string fromTopic = "/home/manoj/applicants_amr_example_1";
        std::ifstream fin_config(fromTopic + "/configuration/products.yaml", std::fstream::in);
        YAML::Node config = YAML::Load(fin_config);

        for (YAML::iterator it_config = config.begin(); it_config != config.end(); ++it_config) {
            YAML::Node configNode = *it_config;
            if (configNode["id"].as<float>() == id) {
                //Iterate through parts
                for (size_t num = 0; num < configNode["parts"].size(); num++) {
                    product.productId = id;
                    product.partName = configNode["parts"][num]["part"].as<std::string>();
                    product.Cx = configNode["parts"][num]["cx"].as<float>();
                    product.Cy = configNode["parts"][num]["cy"].as<float>();
                    product.tag = tag++;
                    Fetch.push_back(product);
                }
            }
        }
    }

    void output(std::vector<Product> Fetch, float Destination_x, float Destination_y)
    {

        size_t min_Index;
        bool* visited = new bool[Fetch.size()];
        float* distance = new float[Fetch.size()];
        for (size_t i = 0; i < Fetch.size(); i++) {
            visited[i] = false;
        }

        for (size_t i = 0; i < Fetch.size(); i++) {

            for (size_t j = 0; j < Fetch.size(); j++) {
                float x1 = source_cx, y1 = source_cy, x2 = Fetch[j].Cx, y2 = Fetch[j].Cy;
                if ((visited[j] == true)) {
                    distance[j] = MAX;
                }
                else {
                    distance[j] = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
                }
            }
            float min = distance[0];
            for (size_t k = 0; k < Fetch.size(); k++) {
                if (distance[k] <= min) {
                    min = distance[k];
                    min_Index = k;
                }
            }

            visited[min_Index] = true;
            source_cx = Fetch[min_Index].Cx;
            source_cy = Fetch[min_Index].Cy;
            Path.push_back(min_Index);
        }
        std::cout << "--------------------------------------------------------" << '\n';
        std::cout << "Working on Order " << orderId << "( " << order_description << " )" << '\n';
        int tag = 1;
        std::cout << "size of path is " << Path.size() << '\n';
        for (size_t i = 0; i < Path.size(); i++) {
            size_t scope = Path[i];
            std::cout << tag << ". Fetching " << Fetch[scope].partName
                      << "for product" << Fetch[scope].productId << "at x: " << Fetch[scope].Cx
                      << ", y: " << Fetch[scope].Cy << '\n';
            tag++;
        }
        std::cout << tag << ". Delivering to destination x: " << Destination_x << ", y: " << Destination_y << '\n';
        std::cout << "__________________________________________________________" << '\n';
        //rclcpp::shutdown();
        delete[] distance;
        delete[] visited;

        Path.clear();
    }
    void publish_marker(std::vector<Product> Fetch)
    {
        marker.header.frame_id = "/frame";
        rcutils_time_point_value_t time_now;
        if (rcutils_system_time_now(&time_now) != RCUTILS_RET_OK) {
        }
        marker.header.stamp.sec = RCL_NS_TO_S(time_now);
        marker.header.stamp.nanosec = time_now;
        marker.lifetime = rclcpp::Duration(100000000000);
        marker.ns = "AMR";
        marker.id = orderId;
        marker.type = shape;
        marker.action = visualization_msgs::msg::Marker::ADD;
        marker.color.r = 1.0;
        marker.color.a = 1.0;
        marker.pose.position.x = source_cx;
        marker.pose.position.y = source_cy;
        marker.pose.position.z = 0;
        marker.pose.orientation.x = 0.0;
        marker.pose.orientation.y = 0.0;
        marker.pose.orientation.z = 0.0;
        marker.pose.orientation.w = 1.0;
        marker.scale.x = 1.0;
        marker.scale.y = 1.0;
        marker.scale.z = 1.0;
        marker.text = orderId;
        markerArray.markers.push_back(marker);
        for (size_t i = 0; i < Fetch.size(); i++) {
            shape = visualization_msgs::msg::Marker::CUBE;
            marker.header.frame_id = "/frame";
            rcutils_time_point_value_t time_now;
            if (rcutils_system_time_now(&time_now) != RCUTILS_RET_OK) {
            }
            marker.header.stamp.sec = RCL_NS_TO_S(time_now);
            marker.header.stamp.nanosec = time_now;
            marker.lifetime = rclcpp::Duration(100000000000);
            marker.ns = "Parts";
            marker.id = Fetch[i].tag;
            marker.type = shape;
            marker.action = visualization_msgs::msg::Marker::ADD;
            marker.color.r = 1.0;
            marker.color.a = 1.0;
            marker.pose.position.x = Fetch[i].Cx;
            marker.pose.position.y = Fetch[i].Cy;
            marker.pose.position.z = 0;
            marker.pose.orientation.x = 0.0;
            marker.pose.orientation.y = 0.0;
            marker.pose.orientation.z = 0.0;
            marker.pose.orientation.w = 1.0;
            marker.scale.x = 1.0;
            marker.scale.y = 1.0;
            marker.scale.z = 1.0;
            marker.text = Fetch[i].partName;
            markerArray.markers.push_back(marker);
            publishe_array->publish(markerArray);
        }
    }

private:
    rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr subscribe_position;
    rclcpp::Subscription<custom_msg::msg::OrderDesc>::SharedPtr subscribe_order;
    rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr publishe_array;
    rclcpp::Publisher<visualization_msgs::msg::Marker>::SharedPtr markerPublish;
    size_t count_;
};

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    std::shared_ptr<Order_optimizer> node(std::make_shared<Order_optimizer>());
    while (rclcpp::ok()) {
        rclcpp::spin_some(node);
        if (orderId != 0)
            node->parse(orderId);
    }
    rclcpp::shutdown();
    return 0;
}