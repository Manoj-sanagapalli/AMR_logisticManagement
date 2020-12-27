#include <iostream>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "custom_msg/msg/order_desc.hpp"
#include <sstream>
#include <fstream>
#include "yaml-cpp/yaml.h"
#include <string>
#include <experimental/filesystem>

namespace fs= std::experimental::filesystem;

//static geometry_msgs::msg::PoseStamped get_currentPosition;
float x=0,y=0,z=0;
float orderId=0;
std::string order_description;
const YAML::Node Load;

struct Order
{
        /* data */
        float Id;
        float cx;
        float cy;
        std::vector<float> products;
};

struct Parts
{
        std::string partName;
        float Pcx;
        float Pcy;
};

struct Configuration
{
      float product_Id;
      std::string pdtName;
      std::vector<Parts> pdtParts;

};

void getParts(Order);

// void operator >> (const YAML::Node& Node, Order& o){
//         Node[0] >> o.Id;
//         Node[1] >> o.cx;
// }

static custom_msg::msg::OrderDesc get_order;
std::string filepath;
std::string default_value = "/home/roscon";
using namespace std::chrono_literals;

class Order_optimizer : public rclcpp::Node
{
        public:
        Order_optimizer()
        : Node("Order_optimizer"),count_(0)
        {
                //Parameter declaration, assigning defaultvalue, reading from commandline
                this->declare_parameter("absolute_path",default_value);
                if (this->has_parameter("absolute_path"))
                {
                        RCLCPP_INFO(this->get_logger(),"parameter exists");
                }
                this->get_parameter("absolute_path",filepath);
                //RCLCPP_INFO(this->get_logger(),"the file path is %s", filepath.c_str());
                //std::cout<<"print statement, absoulute path ="<<filepath<<std::endl;

                subscribe_position= this->create_subscription<geometry_msgs::msg::PoseStamped>(
                        "currentPosition",10,
                        [this](geometry_msgs::msg::PoseStamped::SharedPtr get_currentPosition){
                                x = get_currentPosition->pose.position.x;
                                y = get_currentPosition->pose.position.y;
                                z = get_currentPosition->pose.position.z;
                                //std::cout<<"the values = "<<x<<y<<z<<std::endl;
                                RCLCPP_INFO(this->get_logger(), "position = %f",x);
                        }

                );

                subscribe_order= this->create_subscription<custom_msg::msg::OrderDesc>(
                        "nextOrder",10,
                        [this](custom_msg::msg::OrderDesc::SharedPtr get_orderId){
                                orderId=get_orderId->order_id;
                                order_description=get_orderId->description;
                                //std::cout<<"order description ="<<order_description<<std::endl;
                                //std::cout<<"order id ="<<orderId<<std::endl;

                        }
                );
        }

        void parse()
        {
            try
            {
                    std::string fromTopic = "/home/manoj/applicants_amr_example_1";//comes from topic
                    std::vector<std::string> file;
                    //get all the files in the given folder
                    for (const auto & entry : fs::directory_iterator(fromTopic+"/orders"))
                        file.push_back(entry.path());
                    //Itereate through each and every file inside the given absolute location
                    for (size_t p = 0; p < file.size(); p++)
                    {
                        std::cout<<file[p]<<'\n';
                        //Input the file to istream
                        std::ifstream fin(file[p],std::fstream::in);
                        // Input the yaml file as Node object
                        YAML::Node node = YAML::Load(fin);
                        //std::cout<<"the size of the doc is \t"<<node.size()<<std::endl;

                        for(YAML::iterator iterator = node.begin(); iterator != node.end(); ++iterator)
                        {
                                YAML::Node orderNode = *iterator;
                                if (orderNode["order"].as<float>() == 1200002)
                                {
                                        Order order;
                                        order.Id = orderNode["order"].as<float>();
                                        order.cx = orderNode["cx"].as<float>();
                                        order.cy = orderNode["cy"].as<float>();
                                        std::cout<<"the number of products is "<<orderNode["products"].size()<<'\n';
                                        for(size_t j=0; j< orderNode["products"].size(); ++j)
                                        {
                                                order.products.push_back(orderNode["products"][j].as<float>());
                                              //  std::cout<<"the products are :"<<order.products[j]<<'\n';
                                                
                                        }
                                        getParts(order);
                                        
                                }
                                                          
                        }
                        // //Iterate through all the nodes in the file
                        // for (size_t i = 0; i < node.size(); i++)
                        // {
                        //         //Check for the required order
                        //         if (node[i]["order"].as<float>() == 1200002)
                        //         {
                        //                 std::cout<<"the index is"<<i<<'\n';
                        //                 Order order;
                        //                 order.Id = node[i]["order"].as<float>();
                        //                 order.cx = node[i]["cx"].as<float>();
                        //                 order.cy = node[i]["cy"].as<float>();
                        //                 std::cout<<"the number of products is "<<node[i]["products"].size()<<'\n';
                        //                 for(size_t j=0; j< node[i]["products"].size(); ++j)
                        //                 {
                        //                         //     std::cout<<"the products are :"<<node[i]["products"][j].as<int>()
                        //                         //     <<'\n';
                        //                         order.products.push_back(node[i]["products"][j].as<int>());
                        //                         std::cout<<"the products are :"<<order.products[j]
                        //                         <<'\n';
                        //                 }
                        //                 //std::cout<<"i = \t"<<i<<", Id = "<<order.Id<<", cx="<<order.cx
                        //                 //<<"the number of products is "<<node[i]["products"].size()<<'\n';
                            
                        //         } 
                       
                            
                        // }

                    }
                    
            }
            catch(const YAML::ParserException& e)
            {
                    std::cerr << e.what() << '\n';
            }
            
        }

        void getParts(Order order)
        {
                std::string fromTopic = "/home/manoj/applicants_amr_example_1";
                std::ifstream fin_config(fromTopic+"/configuration/products.yaml",std::fstream::in); 
                YAML::Node config = YAML::Load(fin_config);
                // Iterating through all the products
                //for(size_t i= 0;i<order.products.size();i++)
                //{
                        //std::cout<<"the products are :"<<order.products[i]<<'\n';
                        for (YAML::iterator it_config = config.begin(); it_config != config.end(); ++it_config)
                        {
                                YAML::Node configNode = *it_config;
                                //std::cout<<configNode<<'\n'; //prints all the data of node
                                if (configNode["id"].as<float>() == 9)
                                {
                                        Parts parts;
                                        Configuration configuration;
                                        configuration.product_Id = configNode["id"].as<float>();
                                        
                                        for (size_t num = 0; num < configNode["parts"].size(); num++)
                                        {
                                                //std::cout<<"the part name is"<< configNode["parts"][num]["part"]<<'\n';        
                                                parts.partName = configNode["parts"][num]["part"].as<std::string>();
                                                parts.Pcx = configNode["parts"][num]["cx"].as<float>();
                                                parts.Pcy = configNode["parts"][num]["cy"].as<float>();
                                                configuration.pdtParts.push_back(parts);
                                        }

                                        for (size_t i = 0; i < configuration.pdtParts.size(); i++)
                                        {
                                                std::cout<<"The \t part is"<<configuration.pdtParts[i].partName<<'\n';
                                                std::cout<<"The \t part cx is"<<configuration.pdtParts[i].Pcx<<'\n';
                                                std::cout<<"The \t part cy is"<<configuration.pdtParts[i].Pcy<<'\n';
                                        }
                                        
                                        
                                }
                                
                        }
                        
                                                

                //}
           
                 

        }

        void output(std::vector<Product> Fetch)
        {
                source_cx=775.62494; source_cy=48.782112; //from topic
                bool* visited = new bool[Fetch.size()];
                float* distance = new float[Fetch.size()];
                for (size_t i = 0; i < Fetch.size(); i++)
                {
                        visited[i]=false;
                }
                
                for (size_t i = 0; i < Fetch.size(); i++)
                {
                        // std::cout<<Fetch[i].tag<<". Fetching "<<Fetch[i].partName<<"for product"
                        // <<Fetch[i].productId<<"at x: "<<Fetch[i].Cx<<", y: "<<Fetch[i].Cy
                        // <<'\n';
                        for (size_t i = 0; i < Fetch.size(); i++)
                        {
                                float x1=source_cx,y1=source_cy,x2=Fetch[i].Cx,y2=Fetch[i].Cy;
                                if (visited[i] == true)
                                {
                                        distance[i]= MAX;
                                }
                                else
                                {
                                        distance[i]=sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2)); 
                                }
                                
                        }
                        float min = distance[0]; size_t min_Index;
                        for (size_t i = 0; i < Fetch.size(); i++)
                        {
                                if (distance[i]<min)
                                {
                                        min = distance[i]; min_Index=i;
                                }
                                
                        }
                        visited[min_Index] = true;
                        source_cx= Fetch[min_Index].Cx;
                        source_cy= Fetch[min_Index].Cy;
                        Path.push_back(min_Index);       
                }
                for (size_t i = 0; i < Path.size(); i++)
                {
                        size_t scope= Path[i];
                        std::cout<<Fetch[scope].tag<<". Fetching "<<Fetch[scope].partName
                        <<"for product"<<Fetch[scope].productId<<"at x: "<<Fetch[scope].Cx
                        <<", y: "<<Fetch[scope].Cy<<'\n';
                }
                
        }

        private:
        rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr subscribe_position;
        rclcpp::Subscription<custom_msg::msg::OrderDesc>::SharedPtr subscribe_order;
        size_t count_;
        

};

int main(int argc, char * argv[])
{
        rclcpp::init(argc, argv);
        std::shared_ptr<Order_optimizer>node(std::make_shared<Order_optimizer>());
        //while (rclcpp::ok())
        do
        {
                node->parse();  
        } while (!(rclcpp::ok()));
        
        //{
          //      rclcpp::spin_some(node);
            //    node->parse();
        //}
        rclcpp::shutdown();
        return 0;
}


/*
backup
 void parse()
        {
            try
            {
                    std::ifstream fin("/home/manoj/test.yaml",std::fstream::in);
                    YAML::Node node = YAML::Load(fin);
                    
                    //YAML::LoadFile("/home/manoj/test.yaml");
                    std::cout<<"the size of the doc is \t"<<node.size()<<std::endl;
                    for (size_t i = 0; i < node.size(); i++)
                    {
                            Order order;
                            order.Id = node[i]["order"].as<float>();
                            order.cx = node[i]["cx"].as<float>();
                            order.cy = node[i]["cy"].as<float>();
                            std::cout<<"the number of products is "<<node[i]["products"].size()<<'\n';
                            for(size_t j=0; j< node[i]["products"].size(); ++j)
                            {
                                //     std::cout<<"the products are :"<<node[i]["products"][j].as<int>()
                                //     <<'\n';
                                     order.products.push_back(node[i]["products"][j].as<int>());
                                     std::cout<<"the products are :"<<order.products[j]
                                     <<'\n';
                            }
                            //std::cout<<"i = \t"<<i<<", Id = "<<order.Id<<", cx="<<order.cx
                            //<<"the number of products is "<<node[i]["products"].size()<<'\n';
                            
                            
                    }
                    
            }
            catch(const YAML::ParserException& e)
            {
                    std::cerr << e.what() << '\n';
            }
*/