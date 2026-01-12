import struct
import socket

# === 配置（必须与 C++ 端一致）===
MAGIC = 0xabcdef00
MESSAGE_TYPE_ORDER = 1
HEADER_SIZE = 7  # 4 (magic) + 2 (len) + 1 (type)

HOST = '192.168.1.21'
PORT = 9999

# === 序列化：Order -> bytes (73B) ===
def serialize_order(user_id: str, order_id: str, side: int, price: float, quantity: int, timestamp: int) -> bytes:
    buf = bytearray(73)
    
    # user_id: 15 bytes at [0, 15)
    uid_bytes = user_id.encode('utf-8')[:15]
    buf[0:15] = uid_bytes.ljust(15, b'\x00')
    
    # order_id: 31 bytes at [16, 47)
    oid_bytes = order_id.encode('utf-8')[:31]
    buf[16:47] = oid_bytes.ljust(31, b'\x00')
    
    # side: 1 byte at offset 47
    buf[48] = side
    
    # price: double at 48 (8 bytes)
    struct.pack_into('<d', buf, 49, price)
    
    # quantity: int32 at 56 (4 bytes)
    struct.pack_into('<i', buf, 57, quantity)
    
    # remaining_quantity: int32 at 60 (4 bytes)
    struct.pack_into('<i', buf, 61, quantity)
    
    # timestamp: uint64 at 64 (8 bytes)
    struct.pack_into('<Q', buf, 63, timestamp)
    
    return bytes(buf)

def serialize_cancel_order(order_id: str) -> bytes:
    buf = bytearray(32)
    oid_bytes = order_id.encode('utf-8')[:31]
    buf[0:31] = oid_bytes.ljust(31, b'\x00')
    return bytes(buf)

# === 反序列化：-> dict ===
def deserialize_order(payload: bytes) -> dict:
    order_id = payload[0:31].split(b'\x00', 1)[0].decode('utf-8')
    
    # side
    side_int = payload[32]
    side_str = "BUY" if side_int == 1 else "SELL"  # 根据你的 OrderSide 调整
    
    # quantity
    quantity = struct.unpack_from('<i', payload, 33)[0]
    
    return {
        "order_id": order_id,
        "side": side_str,
        "quantity": quantity,
    }

# === 编码完整消息 ===
def encode_message(msg_type: int, payload: bytes) -> bytes:
    length = len(payload)
    header = struct.pack('<IH', MAGIC, length)
    return header + struct.pack('B', msg_type) + payload

# === 解码完整消息 ===
def decode_message(data: bytes):
    if len(data) < HEADER_SIZE:
        raise ValueError("Message too short for header")
    
    # 解析 header
    magic, length = struct.unpack_from('<IH', data, 0)
    msg_type = data[6]
    
    if magic != MAGIC:
        raise ValueError(f"Invalid magic: expected {MAGIC:08x}, got {magic:08x}")
    
    payload = data[HEADER_SIZE:HEADER_SIZE + length]
    if len(payload) != length:
        raise ValueError(f"Payload truncated: expected {length}, got {len(payload)}")
    
    return {
        "magic": magic,
        "length": length,
        "type": msg_type,
        "payload": payload
    }

# === 主测试逻辑 ===
def add_order():
    # 构造测试订单
    test_data = {
        'user_id': 'trader_006',
        'order_id': 'O3',
        'side':0, 
        'price':2.365,
        'quantity': 33300,
        'timestamp': 1731691200000000  # 微秒时间戳
    }
    
    print("📦 Sending order:")
    print(f"  User: {test_data['user_id']}")
    print(f"  Order ID: {test_data['order_id']}")
    print(f"  Side: {'BUY' if test_data['side'] == 0 else 'SELL'}")
    print(f"  Price: {test_data['price']}")
    print(f"  Quantity: {test_data['quantity']}")
    print()
    
    # 序列化并编码
    payload = serialize_order(**test_data)
    full_msg = encode_message(MESSAGE_TYPE_ORDER, payload)
    
    try:
        # 发送并接收（假设服务端原样返回）
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((HOST, PORT))
            s.sendall(full_msg)
            print("✅ Sent to server.")
            
            # 接收响应（简单起见，假设一次 recv 拿到全部）
            response = s.recv(1024)
            print("Response:", response.hex())
            if not response:
                print("❌ No response from server.")
                return
            
            print(f"\n📥 Received {len(response)} bytes from server.")
            
            # 解析响应
            decoded = decode_message(response)
            
            # 反序列化订单
            order_dict = deserialize_order(decoded['payload'])
            
            print("\n📄 Parsed order:")
            for k, v in order_dict.items():
                print(f"  {k}: {v}")
                
    except Exception as e:
        print(f"❌ Error: {e}")

def cancel_order():
    payload =serialize_cancel_order('O2')
    full_msg = encode_message(2, payload)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        s.sendall(full_msg)
        print("✅ Sent to server.")
        
        # 接收响应（简单起见，假设一次 recv 拿到全部）
        response = s.recv(1024)
        print("Response:", response.hex())
        if not response:
            print("❌ No response from server.")
            return
        
        print(f"\n📥 Received {len(response)} bytes from server.")
        
        # 解析响应
        decoded = decode_message(response)
        
        # 反序列化订单
        order_dict = deserialize_order(decoded['payload'])
        
        print("\n📄 Parsed order:")
        for k, v in order_dict.items():
            print(f"  {k}: {v}")
if __name__ == '__main__':
    cancel_order()
