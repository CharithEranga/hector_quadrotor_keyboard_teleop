#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <signal.h>
#include <termios.h>
#include <stdio.h>
#include <string>

#define KEYCODE_UP_ZP 0x41
#define KEYCODE_DOWN_ZN 0x42
#define KEYCODE_RIGHT_ZC 0x43
#define KEYCODE_LEFT_ZAC 0x44

#define KEYCODE_W_YP 0x77
#define KEYCODE_S_YN 0x73
#define KEYCODE_A_XN 0x61
#define KEYCODE_D_XP 0x64

#define KEYCODE_SPACE_STOP 0x20

struct HexCharStruct
{
    unsigned char c;
    HexCharStruct(unsigned char _c) : c(_c) {}
};

inline std::ostream &operator<<(std::ostream &o, const HexCharStruct &hs)
{
    return (o << std::hex << (int)hs.c);
}

inline HexCharStruct hex(unsigned char _c)
{
    return HexCharStruct(_c);
}

class TeleopQuadrotor
{

  public:
    TeleopQuadrotor();
    void keyLoop();

  private:
    double linear_, angular_, l_scale_, a_scale_;
    ros::NodeHandle nh_;
    ros::Publisher twist_pub_;
};

TeleopQuadrotor::TeleopQuadrotor() : linear_(0),
                                     angular_(0),
                                     l_scale_(1.0),
                                     a_scale_(1.0)
{
    nh_.param("scale_angular", a_scale_, a_scale_);
    nh_.param("scale_linear", l_scale_, l_scale_);

    twist_pub_ = nh_.advertise<geometry_msgs::Twist>("/cmd_vel", 1);
}

int kfd = 0;
struct termios cooked, raw;

void quit(int sig)
{
    (void)sig;
    tcsetattr(kfd, TCSANOW, &cooked);
    ros::shutdown();
    exit(0);
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "teleop_quadrotor");
    TeleopQuadrotor teleop_quadrotor;

    signal(SIGINT, quit);

    teleop_quadrotor.keyLoop();

    return (0);
}

void TeleopQuadrotor::keyLoop()
{
    char c;
    bool dirty = false;

    // get the console in raw mode
    tcgetattr(kfd, &cooked);
    memcpy(&raw, &cooked, sizeof(struct termios));
    raw.c_lflag &= ~(ICANON | ECHO);

    // Setting a new line, then end of file
    raw.c_cc[VEOL] = 1;
    raw.c_cc[VEOF] = 2;
    tcsetattr(kfd, TCSANOW, &raw);

    puts("Reading from keyboard");
    puts("---------------------------");
    puts("Following keys will send continues commands to quadcopter.");
    puts("Use A,D keys to move the quadcopter in X direction.");
    puts("Use W,S keys to move the quadcopter in Y direction.");
    puts("Use UP,DOWN arrow keys to move the quadcopter in Z direction.");
    puts("Use LEFT,RIGHT arrow keys to turn the quadcopter in Z axis.");
    puts("Use SPACE key to stop the commands.");

    for (;;)
    {
        // get the next event from the keyboard
        if (read(kfd, &c, 1) < 0)
        {
            perror("read():");
            exit(-1);
        }

        linear_ = angular_ = 0;
        ROS_DEBUG("value: 0x%02X\n", c);

        int a = (int)c;
        std::cout << hex(a) << std::endl;

        geometry_msgs::Twist twist;

        switch (c)
        {
        case KEYCODE_D_XP:
            ROS_DEBUG("X+");
            twist.linear.x = l_scale_ * 1.0;
            dirty = true;
            break;

        case KEYCODE_A_XN:
            ROS_DEBUG("X-");
            twist.linear.x = l_scale_ * (-1.0);
            dirty = true;
            break;

        case KEYCODE_W_YP:
            ROS_DEBUG("Y+");
            twist.linear.y = l_scale_ * 1.0;
            dirty = true;
            break;

        case KEYCODE_S_YN:
            ROS_DEBUG("Y-");
            twist.linear.y = l_scale_ * (-1.0);
            dirty = true;
            break;

        case KEYCODE_UP_ZP:
            ROS_DEBUG("Z+");
            twist.linear.z = l_scale_ * 1.0;
            dirty = true;
            break;

        case KEYCODE_DOWN_ZN:
            ROS_DEBUG("Z-");
            twist.linear.z = l_scale_ * (-1.0);
            dirty = true;
            break;

        case KEYCODE_RIGHT_ZC:
            ROS_DEBUG("Z_A+");
            twist.angular.z = l_scale_ * 1.0;
            dirty = true;
            break;

        case KEYCODE_LEFT_ZAC:
            ROS_DEBUG("Z_A-");
            twist.angular.z = l_scale_ * (-1.0);
            dirty = true;
            break;

        case KEYCODE_SPACE_STOP:
            ROS_DEBUG("Z_A-");
            twist.linear.x = 0;
            twist.linear.y = 0;
            twist.linear.z = 0;

            twist.angular.x = 0;
            twist.angular.y = 0;
            twist.angular.z = 0;
            dirty = true;
            break;
        }

        if (dirty == true)
        {
            twist_pub_.publish(twist);
            dirty = false;
        }

        // geometry_msgs::Twist twistOff;

        // twist.linear.x = 0;
        // twist.linear.y = 0;
        // twist.linear.z = 0;
        // twist.angular.x = 0;
        // twist.angular.y = 0;
        // twist.angular.z = 0;

        // twist_pub_.publish(twistOff);
    }

    return;
}
