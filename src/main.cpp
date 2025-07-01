#include "Application.h"
int main()
{
	Application app(1920, 1080, "Ray Tracer", true, true, "demo");
	app.Run();
}