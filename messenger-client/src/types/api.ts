export interface LoginResponse {
  token: string;
  user_id: string;
}

export interface RegisterResponse {
  token: string;
  user_id: string;
}

export interface FindUserResponse {
  user_id: number;
  username?: string;
  user_login: string;
}

export interface ChatListItem {
  id: number;
  user1_id: number;
  user2_id: number;
  create_timestamp: string;
}

export interface MessageResponse {
  msg_id: number;
  chat_id: number;
  sender_id: number;
  text: string;
  send_time: string;
}

export interface ApiError {
  message?: string;
  error?: string;
}