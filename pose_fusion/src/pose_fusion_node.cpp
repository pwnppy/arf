// /// MGRS ///

// #include <rclcpp/rclcpp.hpp>
// #include <geometry_msgs/msg/pose_with_covariance_stamped.hpp>
// #include <geometry_msgs/msg/twist_stamped.hpp>
// #include <Eigen/Dense>
// #include <cmath>
// #include <string>
// #include <sstream>

// class PoseFusionNode : public rclcpp::Node
// {
// public:
//     PoseFusionNode()
//         : Node("pose_fusion_node")
//     {
//         // Subscribers for LiDAR and GNSS pose
//         lidar_pose_sub_ = this->create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>(
//             "/localization/pose_with_covariance", 10,
//             std::bind(&PoseFusionNode::lidarPoseCallback, this, std::placeholders::_1));

//         gnss_pose_sub_ = this->create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>(
//             "/gnss_pose_with_covariance", 10,
//             std::bind(&PoseFusionNode::gnssPoseCallback, this, std::placeholders::_1));

//         // Subscribers for EKF and Filter twist
//         ekf_twist_sub_ = this->create_subscription<geometry_msgs::msg::TwistStamped>(
//             "/localization/pose_twist_fusion_filter/twist", 10,
//             std::bind(&PoseFusionNode::ekfTwistCallback, this, std::placeholders::_1));

//         filter_twist_sub_ = this->create_subscription<geometry_msgs::msg::TwistStamped>(
//             "/filter/twist", 10,
//             std::bind(&PoseFusionNode::filterTwistCallback, this, std::placeholders::_1));

//         // Publisher for final fused pose
//         final_pose_pub_ = this->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>("/final/pose_with_covariance", 10);
//         fused_twist_pub_ = this->create_publisher<geometry_msgs::msg::TwistStamped>("/fused_twist", 10);
//     }

// private:
//     void lidarPoseCallback(const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr lidar_msg)
//     {
//         last_lidar_msg_ = lidar_msg;

//         if (last_gnss_msg_)
//         {
//             fusePoses();
//         }
//     }

//     void gnssPoseCallback(const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr gnss_msg)
//     {
//         last_gnss_msg_ = gnss_msg;

//         if (last_lidar_msg_)
//         {
//             fusePoses();
//         }
//     }

//     void ekfTwistCallback(const geometry_msgs::msg::TwistStamped::SharedPtr ekf_twist_msg)
//     {
//         last_ekf_twist_msg_ = ekf_twist_msg;

//         if (last_filter_twist_msg_)
//         {
//             fuseTwists();
//         }
//     }

//     void filterTwistCallback(const geometry_msgs::msg::TwistStamped::SharedPtr filter_twist_msg)
//     {
//         last_filter_twist_msg_ = filter_twist_msg;

//         if (last_ekf_twist_msg_)
//         {
//             fuseTwists();
//         }
//     }

//     void fusePoses()
//     {
//         Eigen::Vector3d lidar_pos(last_lidar_msg_->pose.pose.position.x, last_lidar_msg_->pose.pose.position.y, last_lidar_msg_->pose.pose.position.z);
//         Eigen::Vector3d gnss_pos = convertGnssToUTM(last_gnss_msg_->pose.pose.position);

//         geometry_msgs::msg::PoseWithCovarianceStamped fused_pose;
//         fused_pose.header.stamp = this->now();
//         fused_pose.header.frame_id = "base_link";

//         fused_pose.pose.pose.position.x = lidar_weight_ * lidar_pos.x() + gnss_weight_ * gnss_pos.x();
//         fused_pose.pose.pose.position.y = lidar_weight_ * lidar_pos.y() + gnss_weight_ * gnss_pos.y();
//         fused_pose.pose.pose.position.z = lidar_weight_ * lidar_pos.z() + gnss_weight_ * gnss_pos.z();

//         fused_pose.pose.pose.orientation = last_lidar_msg_->pose.pose.orientation;

//         for (size_t i = 0; i < 36; ++i)
//         {
//             fused_pose.pose.covariance[i] = lidar_weight_ * last_lidar_msg_->pose.covariance[i] +
//                                             gnss_weight_ * last_gnss_msg_->pose.covariance[i];
//         }

//         final_pose_pub_->publish(fused_pose);
//     }

//     void fuseTwists()
//     {
//         geometry_msgs::msg::TwistStamped fused_twist;
//         fused_twist.header.stamp = this->now();
//         fused_twist.header.frame_id = "base_link";  // Adjust frame_id as needed

//         fused_twist.twist.linear.x = ekf_twist_weight_ * last_ekf_twist_msg_->twist.linear.x +
//                                      filter_twist_weight_ * last_filter_twist_msg_->twist.linear.x;
//         fused_twist.twist.linear.y = ekf_twist_weight_ * last_ekf_twist_msg_->twist.linear.y +
//                                      filter_twist_weight_ * last_filter_twist_msg_->twist.linear.y;
//         fused_twist.twist.linear.z = ekf_twist_weight_ * last_ekf_twist_msg_->twist.linear.z +
//                                      filter_twist_weight_ * last_filter_twist_msg_->twist.linear.z;

//         fused_twist.twist.angular.x = ekf_twist_weight_ * last_ekf_twist_msg_->twist.angular.x +
//                                       filter_twist_weight_ * last_filter_twist_msg_->twist.angular.x;
//         fused_twist.twist.angular.y = ekf_twist_weight_ * last_ekf_twist_msg_->twist.angular.y +
//                                       filter_twist_weight_ * last_filter_twist_msg_->twist.angular.y;
//         fused_twist.twist.angular.z = ekf_twist_weight_ * last_ekf_twist_msg_->twist.angular.z +
//                                       filter_twist_weight_ * last_filter_twist_msg_->twist.angular.z;

//         fused_twist_pub_->publish(fused_twist);
//     }

//     std::string convertGnssToMGRS(const geometry_msgs::msg::Point& gnss_position)
//     {
//         // Convert GNSS to UTM
//         Eigen::Vector3d utm_pos = convertGnssToUTM(gnss_position);

//         // Extract UTM values
//         double easting = utm_pos.x();
//         double northing = utm_pos.y();
//         int zone = static_cast<int>((gnss_position.x + 180.0) / 6.0) + 1;
//         char latitude_band = 'C' + (int)((gnss_position.y + 80.0) / 8.0);

//         // Compute 100,000 meter grid square identifier
//         int easting_100k = static_cast<int>(easting / 100000.0);
//         int northing_100k = static_cast<int>(northing / 100000.0);

//         std::string mgrs_grid_square = gridSquareIdentifier(easting_100k, northing_100k);

//         // Format the MGRS string
//         std::ostringstream mgrs_stream;
//         mgrs_stream << zone << latitude_band << mgrs_grid_square
//                     << std::setw(5) << std::setfill('0') << static_cast<int>(easting - easting_100k * 100000.0)
//                     << std::setw(5) << std::setfill('0') << static_cast<int>(northing - northing_100k * 100000.0);

//         return mgrs_stream.str();
//     }

//     std::string gridSquareIdentifier(int easting_100k, int northing_100k)
//     {
//         static const std::string letters = "ABCDEFGHJKLMNPQRSTUVWXYZ";
//         char easting_letter = letters[easting_100k % 5];
//         char northing_letter = letters[northing_100k % 5];
//         return std::string(1, easting_letter) + northing_letter;
//     }

//     Eigen::Vector3d convertGnssToUTM(const geometry_msgs::msg::Point& gnss_position)
//     {
//         // WGS84 Ellipsoid parameters
//         const double a = 6378137.0; // Semi-major axis
//         const double f = 1.0 / 298.257223563; // Flattening
//         const double b = a * (1.0 - f); // Semi-minor axis
//         const double e_sq = (a * a - b * b) / (a * a); // Square of eccentricity

//         // Convert latitude and longitude to radians
//         double lat_rad = gnss_position.y * M_PI / 180.0;
//         double lon_rad = gnss_position.x * M_PI / 180.0;

//         // Calculate UTM Zone
//         int zone = static_cast<int>((gnss_position.x + 180.0) / 6.0) + 1;

//         // Central meridian for the UTM zone
//         double lon_0 = (zone - 1) * 6.0 - 180.0 + 3.0;
//         double lon_0_rad = lon_0 * M_PI / 180.0;

//         // Ellipsoid parameters
//         double N = a / sqrt(1.0 - e_sq * sin(lat_rad) * sin(lat_rad));
//         double T = tan(lat_rad) * tan(lat_rad);
//         double C = e_sq / (1.0 - e_sq) * cos(lat_rad) * cos(lat_rad);
//         double A = cos(lat_rad) * (lon_rad - lon_0_rad);

//         // Calculate M (the distance along the central meridian)
//         double M = a * ((1.0 - e_sq / 4.0 - 3.0 * e_sq * e_sq / 64.0 - 5.0 * e_sq * e_sq * e_sq / 256.0) * lat_rad -
//                         (3.0 * e_sq / 8.0 + 3.0 * e_sq * e_sq / 32.0 + 45.0 * e_sq * e_sq * e_sq / 1024.0) * sin(2.0 * lat_rad) +
//                         (15.0 * e_sq * e_sq / 256.0 + 45.0 * e_sq * e_sq * e_sq / 1024.0) * sin(4.0 * lat_rad) -
//                         (35.0 * e_sq * e_sq * e_sq / 3072.0) * sin(6.0 * lat_rad));

//         // UTM coordinates
//         double easting = 500000.0 + (N * (A + (1.0 - T + C) * A * A * A / 6.0 +
//                               (5.0 - 18.0 * T + T * T + 72.0 * C - 58.0 * e_sq) * A * A * A * A * A / 120.0));
//         double northing = (M + N * tan(lat_rad) * (A * A / 2.0 +
//                              (5.0 - T + 9.0 * C + 4.0 * C * C) * A * A * A * A / 24.0 +
//                              (61.0 - 58.0 * T + T * T + 600.0 * C - 330.0 * e_sq) * A * A * A * A * A * A / 720.0));

//         // Adjust for southern hemisphere
//         if (gnss_position.y < 0.0)
//         {
//             northing += 10000000.0;
//         }

//         RCLCPP_INFO(this->get_logger(), "UTM Zone: %d, Easting: %f, Northing: %f", zone, easting, northing);

//         // Return as an ENU vector (easting, northing, altitude)
//         return Eigen::Vector3d(easting, northing, gnss_position.z);
//     }

//     rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr lidar_pose_sub_;
//     rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr gnss_pose_sub_;
//     rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr ekf_twist_sub_;
//     rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr filter_twist_sub_;
//     rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr final_pose_pub_;
//     rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr fused_twist_pub_;

//     geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr last_lidar_msg_;
//     geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr last_gnss_msg_;
//     geometry_msgs::msg::TwistStamped::SharedPtr last_ekf_twist_msg_;
//     geometry_msgs::msg::TwistStamped::SharedPtr last_filter_twist_msg_;

//     double lidar_weight_ = 0.0; // Adjust weights as needed
//     double gnss_weight_ = 1.0;
//     double ekf_twist_weight_ = 0.5;
//     double filter_twist_weight_ = 0.5;
// };

// int main(int argc, char *argv[])
// {
//     rclcpp::init(argc, argv);
//     rclcpp::spin(std::make_shared<PoseFusionNode>());
//     rclcpp::shutdown();
//     return 0;
// }










#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose_with_covariance_stamped.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>
#include <Eigen/Dense>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>

class PoseFusionNode : public rclcpp::Node
{
public:
    PoseFusionNode()
        : Node("pose_fusion_node")
    {
        // Subscribers for LiDAR and GNSS pose
        lidar_pose_sub_ = this->create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>(
            "/localization/pose_with_covariance", 10,
            std::bind(&PoseFusionNode::lidarPoseCallback, this, std::placeholders::_1));

        gnss_pose_sub_ = this->create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>(
            "/gnss_pose_with_covariance", 10,
            std::bind(&PoseFusionNode::gnssPoseCallback, this, std::placeholders::_1));

        // Subscribers for EKF and Filter twist
        ekf_twist_sub_ = this->create_subscription<geometry_msgs::msg::TwistStamped>(
            "/localization/pose_twist_fusion_filter/twist", 10,
            std::bind(&PoseFusionNode::ekfTwistCallback, this, std::placeholders::_1));

        filter_twist_sub_ = this->create_subscription<geometry_msgs::msg::TwistStamped>(
            "/filter/twist", 10,
            std::bind(&PoseFusionNode::filterTwistCallback, this, std::placeholders::_1));

        // Publisher for final fused pose
        final_pose_pub_ = this->create_publisher<geometry_msgs::msg::PoseWithCovarianceStamped>("/final/pose_with_covariance", 10);
        fused_twist_pub_ = this->create_publisher<geometry_msgs::msg::TwistStamped>("/fused_twist", 10);
    }

private:
    void lidarPoseCallback(const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr lidar_msg)
    {
        last_lidar_msg_ = lidar_msg;

        if (last_gnss_msg_)
        {
            fusePoses();
        }
    }

    void gnssPoseCallback(const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr gnss_msg)
    {
        last_gnss_msg_ = gnss_msg;

        if (last_lidar_msg_)
        {
            fusePoses();
        }
    }

    void ekfTwistCallback(const geometry_msgs::msg::TwistStamped::SharedPtr ekf_twist_msg)
    {
        last_ekf_twist_msg_ = ekf_twist_msg;

        if (last_filter_twist_msg_)
        {
            fuseTwists();
        }
    }

    void filterTwistCallback(const geometry_msgs::msg::TwistStamped::SharedPtr filter_twist_msg)
    {
        last_filter_twist_msg_ = filter_twist_msg;

        if (last_ekf_twist_msg_)
        {
            fuseTwists();
        }
    }

    void fusePoses()
    {
        Eigen::Vector3d lidar_pos(last_lidar_msg_->pose.pose.position.x, last_lidar_msg_->pose.pose.position.y, last_lidar_msg_->pose.pose.position.z);
        Eigen::Vector3d gnss_pos = convertGnssToUTM(last_gnss_msg_->pose.pose.position);

        geometry_msgs::msg::PoseWithCovarianceStamped fused_pose;
        fused_pose.header.stamp = this->now();
        fused_pose.header.frame_id = "base_link";

        fused_pose.pose.pose.position.x = lidar_weight_ * lidar_pos.x() + gnss_weight_ * gnss_pos.x();
        fused_pose.pose.pose.position.y = lidar_weight_ * lidar_pos.y() + gnss_weight_ * gnss_pos.y();
        fused_pose.pose.pose.position.z = lidar_weight_ * lidar_pos.z() + gnss_weight_ * gnss_pos.z();

        fused_pose.pose.pose.orientation = last_lidar_msg_->pose.pose.orientation;

        for (size_t i = 0; i < 36; ++i)
        {
            fused_pose.pose.covariance[i] = lidar_weight_ * last_lidar_msg_->pose.covariance[i] +
                                            gnss_weight_ * last_gnss_msg_->pose.covariance[i];
        }

        final_pose_pub_->publish(fused_pose);
    }

    void fuseTwists()
    {
        geometry_msgs::msg::TwistStamped fused_twist;
        fused_twist.header.stamp = this->now();
        fused_twist.header.frame_id = "base_link";  // Adjust frame_id as needed

        fused_twist.twist.linear.x = ekf_twist_weight_ * last_ekf_twist_msg_->twist.linear.x +
                                     filter_twist_weight_ * last_filter_twist_msg_->twist.linear.x;
        fused_twist.twist.linear.y = ekf_twist_weight_ * last_ekf_twist_msg_->twist.linear.y +
                                     filter_twist_weight_ * last_filter_twist_msg_->twist.linear.y;
        fused_twist.twist.linear.z = ekf_twist_weight_ * last_ekf_twist_msg_->twist.linear.z +
                                     filter_twist_weight_ * last_filter_twist_msg_->twist.linear.z;

        fused_twist.twist.angular.x = ekf_twist_weight_ * last_ekf_twist_msg_->twist.angular.x +
                                      filter_twist_weight_ * last_filter_twist_msg_->twist.angular.x;
        fused_twist.twist.angular.y = ekf_twist_weight_ * last_ekf_twist_msg_->twist.angular.y +
                                      filter_twist_weight_ * last_filter_twist_msg_->twist.angular.y;
        fused_twist.twist.angular.z = ekf_twist_weight_ * last_ekf_twist_msg_->twist.angular.z +
                                      filter_twist_weight_ * last_filter_twist_msg_->twist.angular.z;

        fused_twist_pub_->publish(fused_twist);
    }

    std::string convertGnssToMGRS(const geometry_msgs::msg::Point& gnss_position)
    {
        // Convert GNSS to UTM
        Eigen::Vector3d utm_pos = convertGnssToUTM(gnss_position);

        // Extract UTM values
        double easting = utm_pos.x();
        double northing = utm_pos.y();
        int zone = static_cast<int>((gnss_position.x + 180.0) / 6.0) + 1;
        char latitude_band = 'C' + (int)((gnss_position.y + 80.0) / 8.0);

        // Compute 100,000 meter grid square identifier
        int easting_100k = static_cast<int>(easting / 100000.0);
        int northing_100k = static_cast<int>(northing / 100000.0);

        std::string mgrs_grid_square = gridSquareIdentifier(easting_100k, northing_100k);

        // Format the MGRS string
        std::ostringstream mgrs_stream;
        mgrs_stream << zone << latitude_band << mgrs_grid_square
                    << std::setw(5) << std::setfill('0') << static_cast<int>(easting - easting_100k * 100000.0)
                    << std::setw(5) << std::setfill('0') << static_cast<int>(northing - northing_100k * 100000.0);

        return mgrs_stream.str();
    }

    std::string gridSquareIdentifier(int easting_100k, int northing_100k)
    {
        static const std::string letters = "ABCDEFGHJKLMNPQRSTUVWXYZ";
        char easting_letter = letters[easting_100k % 5];
        char northing_letter = letters[northing_100k % 5];
        return std::string(1, easting_letter) + northing_letter;
    }

    Eigen::Vector3d convertGnssToUTM(const geometry_msgs::msg::Point& gnss_position)
    {
        // WGS84 Ellipsoid parameters
        const double a = 6378137.0; // Semi-major axis
        const double f = 1.0 / 298.257223563; // Flattening
        const double b = a * (1.0 - f); // Semi-minor axis
        const double e_sq = (a * a - b * b) / (a * a); // Square of eccentricity

        // Convert latitude and longitude to radians
        double lat_rad = gnss_position.y * M_PI / 180.0;
        double lon_rad = gnss_position.x * M_PI / 180.0;

        // Calculate UTM Zone
        int zone = static_cast<int>((gnss_position.x + 180.0) / 6.0) + 1;

        // Central meridian for the UTM zone
        double lon_0 = (zone - 1) * 6.0 - 180.0 + 3.0;
        double lon_0_rad = lon_0 * M_PI / 180.0;

        // Ellipsoid parameters
        double N = a / sqrt(1.0 - e_sq * sin(lat_rad) * sin(lat_rad));
        double T = tan(lat_rad) * tan(lat_rad);
        double C = e_sq / (1.0 - e_sq) * cos(lat_rad) * cos(lat_rad);
        double A = cos(lat_rad) * (lon_rad - lon_0_rad);

        // Calculate M (the distance along the central meridian)
        double M = a * ((1.0 - e_sq / 4.0 - 3.0 * e_sq * e_sq / 64.0 - 5.0 * e_sq * e_sq * e_sq / 256.0) * lat_rad -
                        (3.0 * e_sq / 8.0 + 3.0 * e_sq * e_sq / 32.0 + 45.0 * e_sq * e_sq * e_sq / 1024.0) * sin(2.0 * lat_rad) +
                        (15.0 * e_sq * e_sq / 256.0 + 45.0 * e_sq * e_sq * e_sq / 1024.0) * sin(4.0 * lat_rad) -
                        (35.0 * e_sq * e_sq * e_sq / 3072.0) * sin(6.0 * lat_rad));

        // UTM coordinates
        double easting = 500000.0 + (N * (A + (1.0 - T + C) * A * A * A / 6.0 +
                              (5.0 - 18.0 * T + T * T + 72.0 * C - 58.0 * e_sq) * A * A * A * A * A / 120.0));
        double northing = (M + N * tan(lat_rad) * (A * A / 2.0 +
                             (5.0 - T + 9.0 * C + 4.0 * C * C) * A * A * A * A / 24.0 +
                             (61.0 - 58.0 * T + T * T + 600.0 * C - 330.0 * e_sq) * A * A * A * A * A * A / 720.0));

        // Adjust for southern hemisphere
        if (gnss_position.y < 0.0)
        {
            northing += 10000000.0;
        }

        RCLCPP_INFO(this->get_logger(), "UTM Zone: %d, Easting: %f, Northing: %f", zone, easting, northing);

        // Return as an ENU vector (easting, northing, altitude)
        return Eigen::Vector3d(easting, northing, gnss_position.z);
    }

    rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr lidar_pose_sub_;
    rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr gnss_pose_sub_;
    rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr ekf_twist_sub_;
    rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr filter_twist_sub_;
    rclcpp::Publisher<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr final_pose_pub_;
    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr fused_twist_pub_;

    geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr last_lidar_msg_;
    geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr last_gnss_msg_;
    geometry_msgs::msg::TwistStamped::SharedPtr last_ekf_twist_msg_;
    geometry_msgs::msg::TwistStamped::SharedPtr last_filter_twist_msg_;

    double lidar_weight_ = 0.0; // Adjust weights as needed
    double gnss_weight_ = 1.0;
    double ekf_twist_weight_ = 0.5;
    double filter_twist_weight_ = 0.5;
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<PoseFusionNode>());
    rclcpp::shutdown();
    return 0;
}
