RWStructuredBuffer<uint> g_SampleCounter : register(u15);

[numthreads(1, 1, 1)]
void main(uint3 dispatch_thread_id: SV_DispatchThreadID)
{
    g_SampleCounter[0] = 0;
}
