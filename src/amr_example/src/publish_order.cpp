
#include <iostream>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "custom_msg/msg/order_desc.hpp"
#include <sstream>
#include <fstream>

using namespace std::chrono_literals;
float count = 1;
custom_msg::msg::OrderDesc order;

/* This example creates a subclass of Node and uses std::bind() to register a
* member function as a callback from the timer. */

class PublishOrder : public rclcpp::Node
{
  public:
    PublishOrder()
    : Node("test"), count_(0)
    {
      publisher_ = this->create_publisher<geometry_msgs::msg::PoseStamped>("currentPosition", 1);
      publish_order = this->create_publisher<custom_msg::msg::OrderDesc>("nextOrder",1);
      timer_ = this->create_wall_timer(
      2000ms, std::bind(&PublishOrder::timer_callback, this));
    }

  private:
    void timer_callback()
    {
      auto message = geometry_msgs::msg::PoseStamped();
      message.pose.position.x = 17.263115;
      message.pose.position.y = 977.0855;
      order.order_id=1200015;
      order.description= "test description";
      publisher_->publish(message);
      publish_order->publish(order);
      count++;
    }
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr publisher_;
    rclcpp::Publisher<custom_msg::msg::OrderDesc>::SharedPtr publish_order;
    size_t count_;
  };

  int main(int argc, char * argv[])
  {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<PublishOrder>());
    rclcpp::shutdown();
    return 0;
  }
