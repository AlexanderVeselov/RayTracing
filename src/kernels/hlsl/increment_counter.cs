RWStructuredBuffer<uint> g_Counter : register(u0);

[numthreads(1, 1, 1)]
void main(uint3 dispatch_thread_id: SV_DispatchThreadID)
{
    g_Counter[0] += 1;
}
