namespace Sirikata { namespace QueueBench {
double standardfalloff (const UUID&a, const UUID&b);
extern std::tr1::function<double(const Vector3d&, const Vector3d&)> gLocationPriority;
extern std::tr1::function<double(const UUID&, const UUID&)> gPriority;

} }
