export interface Message {
  sender: string;
  text: string;
  timestamp: number;
  recipient?: string;
}