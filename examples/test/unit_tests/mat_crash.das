// options log=true, logStack=true, logNodes=true, optimize=true

[export]
def test
    var a,b : float3x4
    identity(a)
    identity(b)
    a = a*b
    return true

