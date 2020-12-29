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

// Declaring message variables
visualization_msgs::msg::MarkerArray markerArray;
visualization_msgs::msg::Marker marker;
uint32_t shape = visualization_msgs::msg::Marker::CUBE;
static custom_msg::msg::OrderDesc get_order;


float MAX = std::numeric_limits<float>::max();
// namespace declaration for file i/o 
namespace fs = std::experimental::filesystem;

// Declaring variables to contain order information
uint32_t orderId = 0;
float source_cx = 0, source_cy = 0, current_x = 0, current_y = 0;
std::string order_description;
std::string filepath;
std::string default_value = "/home/roscon";

//const YAML::Node Load;
int tag = 0;

// Declaring structures to contain order and product data
struct Product
{
	float productId;
	std::string partName;
	float Cx;
	float Cy;
	int tag;
};
std::vector<Product> Fetch;
std::vector<size_t> Path;
struct Order
{
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

// Declaring functions
void parse(uint32_t id);
void getParts(float productId);
void output(std::vector<Product> Fetch, float cx, float cy);
void publish_marker(std::vector<Product> Fetch);

// Main class

class Unit_test: public rclcpp::Node
{
	public: Unit_test(): Node("Order_optimizer"), count_(0)
	{
		publishe_array = this->create_publisher<visualization_msgs::msg::MarkerArray > ("order_path", 1);

		//Parameter declaration, assigning defaultvalue, reading from commandline
		this->declare_parameter("absolute_path", default_value);
		if (!(this->has_parameter("absolute_path")))
		{
			RCLCPP_INFO(this->get_logger(), "parameter donot exists");
		}
		this->get_parameter("absolute_path", filepath);
    std::cout<<"The value of the parameter absolute_path is "<<filepath<<"\n";

    orderId= 1200015;
    current_x = 17.263115;
    current_y = 977.0855;
    order_description = "Description of 1200015";

	}

	// Function to parse the data from files 
    void parse(uint32_t orderId)
	{
		std::vector<std::string > file;

		//get all the files in the given folder
		for (const auto &entry: fs::directory_iterator(filepath + "/orders"))
			file.push_back(entry.path());

		//Itereate through each and every file inside the given absolute location
		size_t p = 0;
		bool orderFound = false;
		while ((p < file.size()) && !(orderFound))
		{
			std::cout <<"Searching the file "<< file[p] <<" for the order "<< orderId<<'\n';
			//Input the file to istream
			std::ifstream fin(file[p], std::fstream:: in);
			// Input the yaml file as Node object
			YAML::Node node = YAML::Load(fin);
			
			//Iterate through document
			for (YAML::iterator iterator = node.begin(); iterator != node.end(); ++iterator)
			{
				YAML::Node orderNode = *iterator;

                // check for the required order
				if (orderNode["order"].as<uint32_t> () == orderId)
				{
          std::cout<<"Order found in this file"<< "\n";
					float Destination_x = orderNode["cx"].as<float> ();
					float Destination_y = orderNode["cy"].as<float> ();
					orderFound = true;
          std::cout<<"The destination coordinates for the order "<<orderId<<" are "<<Destination_x
          <<" and "<<Destination_y<<"\n";
          std::cout<<"The products of the order"<<orderId<<"are :"<<"\n";
          for (size_t k = 0; k < orderNode["products"].size(); ++k)
          {
            std::cout<<"  -"<<orderNode["products"][k]<<"\n";
          }
          std::cout<<"\n";

					//Iterate through products once order is found
					for (size_t j = 0; j<orderNode["products"].size(); ++j)
					{
            std::cout<<"The parts of the product "<<orderNode["products"][j]<<" are "<<"\n";

                        // For every product, iterate through products list to get all parts.
						getParts(orderNode["products"][j].as<float> ());
					}
					std::cout <<"\n"<< "Total number of parts to collect are" << Fetch.size() << '\n';

                    // print the fetched data in required format
					output(Fetch, Destination_x, Destination_y);

                    // send the fetched data to rviz in the form of markers
					publish_marker(Fetch);
					Fetch.clear();
				}   
			} //end loop for document iteration

			p++;
		} // end loop for files iteration in the given folder
	}

	void getParts(float id)
	{
		Product product;
		std::string fromTopic = "/home/manoj/applicants_amr_example_1";
		std::ifstream fin_config(fromTopic + "/configuration/products.yaml", std::fstream:: in);

        // Load products.yaml in to a document node
		YAML::Node config = YAML::Load(fin_config);

        // Iterate through the document to parse products
		for (YAML::iterator it_config = config.begin(); it_config != config.end(); ++it_config)
		{
			YAML::Node configNode = *it_config;
			if (configNode["id"].as<float> () == id)
			{
			 	//Iterate through parts
				for (size_t num = 0; num<configNode["parts"].size(); num++)
				{
					product.productId = id;
					product.partName = configNode["parts"][num]["part"].as<std::string > ();
					product.Cx = configNode["parts"][num]["cx"].as<float> ();
					product.Cy = configNode["parts"][num]["cy"].as<float> ();
					product.tag = tag++;
          std::cout<<"    -"<<configNode["parts"][num]["part"]<<"\n";

                    // Push the data of respective product and part to "Fetch" vector
					Fetch.push_back(product);
				}
			}
		}
	}

    // Output function to calculate shortest path and print 
	void output(std::vector<Product> Fetch, float Destination_x, float Destination_y)
	{

        // find the shortest path
		source_cx = current_x;
		source_cy = current_y;
		size_t min_Index;

        // declare distance array and initialize visited to false
		bool *visited = new bool[Fetch.size()];
		float *distance = new float[Fetch.size()];
		for (size_t i = 0; i < Fetch.size(); i++)
		{
			visited[i] = false;
		}

        // iterate through the data in fetch vector
		for (size_t i = 0; i < Fetch.size(); i++)
		{

      std::cout<<"The distances from source to "<<"\n";
			// Calculate minimum distances for every step
            for (size_t j = 0; j < Fetch.size(); j++)
			{
				float x1 = source_cx, y1 = source_cy, x2 = Fetch[j].Cx, y2 = Fetch[j].Cy;
        
				if ((visited[j] == true))
				{
					distance[j] = MAX;
				}
				else
				{
					distance[j] = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
				}
        std::cout<<"  -"<<Fetch[j].partName<<" is "<<distance[j]<<"\n";
			}
			float min = distance[0];
			for (size_t k = 0; k < Fetch.size(); k++)
			{
				if (distance[k] <= min)
				{
					min = distance[k];
					min_Index = k;
				}
			}
      std::cout<<"The part with shortest distance from here is "<<Fetch[min_Index].partName<<"\n";
      
			visited[min_Index] = true;
			source_cx = Fetch[min_Index].Cx;
			source_cy = Fetch[min_Index].Cy;
      std::cout<<"New source moved to the location of "<<Fetch[min_Index].partName<<"=>"
      <<source_cx<<","<<source_cy<< "\n"<<"\n";

            // push the data of minimum distances and corresponding part data
			Path.push_back(min_Index);
		}

        //printing the shortest path data
    std::cout<<"Fetching parts"<<"\n";
		std::cout << "--------------------------------------------------------" << '\n';
		std::cout << "Working on Order " << orderId << "(" << order_description << ")" << '\n';
		int tag = 1;
		for (size_t i = 0; i < Path.size(); i++)
		{
			size_t scope = Path[i];
			std::cout << tag << ". Fetching " << Fetch[scope].partName <<
				"for product" << Fetch[scope].productId << "at x: " << Fetch[scope].Cx <<
				", y: " << Fetch[scope].Cy << '\n';
			tag++;
		}
		std::cout << tag << ". Delivering to destination x: " << Destination_x << ", y: " << Destination_y << '\n';
		std::cout << "__________________________________________________________" << '\n';
		//rclcpp::shutdown();
		delete[] distance;
		delete[] visited;
		Path.clear();
	}

    // publish function to publish the markers
	void publish_marker(std::vector<Product> Fetch)
	{

        // Iterat through data in fetch vector
		for (size_t i = 0; i < Fetch.size(); i++)
		{
            // create markers for parts and AMR
			shape = visualization_msgs::msg::Marker::CUBE;
			marker.header.frame_id = "/frame";
			rcutils_time_point_value_t time_now;
			if (rcutils_system_time_now(&time_now) != RCUTILS_RET_OK) {}
			marker.header.stamp.sec = RCL_NS_TO_S(time_now);
			marker.header.stamp.nanosec = time_now;
			marker.lifetime = rclcpp::Duration(100000000000);
			marker.ns = "Parts";
			marker.id = Fetch[i].tag;
			marker.type = shape;
			marker.action = visualization_msgs::msg::Marker::ADD;
			marker.color.r = 1.0;
			marker.color.g = 0.0;
			marker.color.a = 1.0;
			marker.pose.position.x = Fetch[i].Cx;
			marker.pose.position.y = Fetch[i].Cy;
			marker.pose.position.z = 0;
			marker.pose.orientation.x = 0.0;
			marker.pose.orientation.y = 0.0;
			marker.pose.orientation.z = 0.0;
			marker.pose.orientation.w = 1.0;
			marker.scale.x = 50.0;
			marker.scale.y = 50.0;
			marker.scale.z = 50.0;
			marker.text = Fetch[i].partName;
			markerArray.markers.push_back(marker);
      std::cout<<"Sending marker for "<<Fetch[i].partName<<" to the topic "<<"\n";

			marker.header.frame_id = "/frame";

			marker.header.stamp.sec = RCL_NS_TO_S(time_now);
			marker.header.stamp.nanosec = time_now;
			marker.lifetime = rclcpp::Duration(100000000000);
			marker.ns = "AMR";
			marker.id = orderId;
			marker.type = visualization_msgs::msg::Marker::CYLINDER;
			marker.pose.position.x = current_x;
			marker.pose.position.y = current_y;
			marker.pose.position.z = 0;
			marker.action = visualization_msgs::msg::Marker::ADD;
			marker.color.r = 0.0;
			marker.color.g = 1.0;
			marker.color.a = 1.0;
			marker.pose.position.z = 0;
			marker.pose.orientation.x = 0.0;
			marker.pose.orientation.y = 0.0;
			marker.pose.orientation.z = 0.0;
			marker.pose.orientation.w = 1.0;
			marker.scale.x = 100.0;
			marker.scale.y = 100.0;
			marker.scale.z = 100.0;
		

            // push the markers to marker array and publish marker array
			markerArray.markers.push_back(marker);

			publishe_array->publish(markerArray);
		}
	}

	private: rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr subscribe_position;
	rclcpp::Subscription<custom_msg::msg::OrderDesc>::SharedPtr subscribe_order;
	rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr publishe_array;
	rclcpp::Publisher<visualization_msgs::msg::Marker>::SharedPtr markerPublish;
	size_t count_;
};

int main(int argc, char *argv[])
{
	rclcpp::init(argc, argv);
	std::shared_ptr<Unit_test> node(std::make_shared<Unit_test> ());
	//while (rclcpp::ok())
	//{
		rclcpp::spin_some(node);
		if (orderId != 0)
			node->parse(orderId);
	//}
	rclcpp::shutdown();
	return 0;
}